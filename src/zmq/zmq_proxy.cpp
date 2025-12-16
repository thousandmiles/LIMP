#include "limp/zmq/zmq_proxy.hpp"
#include <iostream>

namespace limp
{

    ZMQProxy::ZMQProxy(ProxyType type, const ZMQConfig &config)
        : type_(type), config_(config), running_(false), stopRequested_(false), 
          frontendBind_(true), backendBind_(true)
    {
        context_ = std::make_unique<zmq::context_t>(config_.ioThreads);
    }

    ZMQProxy::~ZMQProxy()
    {
        stop();
    }

    bool ZMQProxy::setFrontend(const std::string &endpoint, bool bind)
    {
        if (running_.load())
        {
            if (errorCallback_)
            {
                errorCallback_("Cannot set frontend while proxy is running");
            }
            return false;
        }

        frontendEndpoint_ = endpoint;
        frontendBind_ = bind;
        return true;
    }

    bool ZMQProxy::setBackend(const std::string &endpoint, bool bind)
    {
        if (running_.load())
        {
            if (errorCallback_)
            {
                errorCallback_("Cannot set backend while proxy is running");
            }
            return false;
        }

        backendEndpoint_ = endpoint;
        backendBind_ = bind;
        return true;
    }

    bool ZMQProxy::setCapture(const std::string &endpoint)
    {
        if (running_.load())
        {
            if (errorCallback_)
            {
                errorCallback_("Cannot set capture while proxy is running");
            }
            return false;
        }

        captureEndpoint_ = endpoint;
        return true;
    }

    void ZMQProxy::setErrorCallback(std::function<void(const std::string &)> callback)
    {
        errorCallback_ = callback;
    }

    bool ZMQProxy::start()
    {
        if (running_.load())
        {
            if (errorCallback_)
            {
                errorCallback_("Proxy is already running");
            }
            return false;
        }

        if (frontendEndpoint_.empty() || backendEndpoint_.empty())
        {
            if (errorCallback_)
            {
                errorCallback_("Frontend and backend endpoints must be set");
            }
            return false;
        }

        stopRequested_ = false;
        thread_ = std::make_unique<std::thread>(&ZMQProxy::proxyThread, this);
        return true;
    }

    void ZMQProxy::stop()
    {
        if (!running_.load() && !thread_)
        {
            return;
        }

        stopRequested_ = true;

        // Shutdown context to terminate proxy
        if (context_)
        {
            context_->shutdown();
        }

        // Wait for thread to finish
        if (thread_ && thread_->joinable())
        {
            thread_->join();
        }

        thread_.reset();
        running_ = false;

        // Recreate context for potential restart
        context_ = std::make_unique<zmq::context_t>(config_.ioThreads);
    }

    zmq::socket_type ZMQProxy::getFrontendSocketType() const
    {
        switch (type_)
        {
        case ProxyType::ROUTER_DEALER:
        case ProxyType::ROUTER_ROUTER:
            return zmq::socket_type::router;
        case ProxyType::DEALER_DEALER:
            return zmq::socket_type::dealer;
        case ProxyType::XPUB_XSUB:
            return zmq::socket_type::xsub; // Publishers connect to XSUB
        default:
            return zmq::socket_type::router;
        }
    }

    zmq::socket_type ZMQProxy::getBackendSocketType() const
    {
        switch (type_)
        {
        case ProxyType::ROUTER_DEALER:
        case ProxyType::DEALER_DEALER:
            return zmq::socket_type::dealer;
        case ProxyType::ROUTER_ROUTER:
            return zmq::socket_type::router;
        case ProxyType::XPUB_XSUB:
            return zmq::socket_type::xpub; // Subscribers connect to XPUB
        default:
            return zmq::socket_type::dealer;
        }
    }

    void ZMQProxy::handleError(const zmq::error_t &error, const std::string &context)
    {
        if (errorCallback_)
        {
            std::string errorMsg = context;
            if (error.num() != 0)
            {
                errorMsg += ": " + std::string(error.what());
            }
            errorCallback_(errorMsg);
        }
    }

    void ZMQProxy::proxyThread()
    {
        try
        {
            // Create frontend socket
            zmq::socket_t frontend(*context_, getFrontendSocketType());

            // Configure frontend socket
            frontend.set(zmq::sockopt::linger, config_.lingerTime);
            if (config_.sendTimeout >= 0)
            {
                frontend.set(zmq::sockopt::sndtimeo, config_.sendTimeout);
            }
            if (config_.receiveTimeout >= 0)
            {
                frontend.set(zmq::sockopt::rcvtimeo, config_.receiveTimeout);
            }

            // Bind or connect frontend
            if (frontendBind_)
            {
                frontend.bind(frontendEndpoint_);
            }
            else
            {
                frontend.connect(frontendEndpoint_);
            }

            // Create backend socket
            zmq::socket_t backend(*context_, getBackendSocketType());

            // Configure backend socket
            backend.set(zmq::sockopt::linger, config_.lingerTime);
            if (config_.sendTimeout >= 0)
            {
                backend.set(zmq::sockopt::sndtimeo, config_.sendTimeout);
            }
            if (config_.receiveTimeout >= 0)
            {
                backend.set(zmq::sockopt::rcvtimeo, config_.receiveTimeout);
            }

            // Bind or connect backend
            if (backendBind_)
            {
                backend.bind(backendEndpoint_);
            }
            else
            {
                backend.connect(backendEndpoint_);
            }

            running_ = true;

            // Run proxy with optional capture socket
            if (!captureEndpoint_.empty())
            {
                zmq::socket_t capture(*context_, zmq::socket_type::pub);
                capture.bind(captureEndpoint_);

                // Run proxy with capture (blocks until context terminated)
                zmq::proxy(zmq::socket_ref(frontend), zmq::socket_ref(backend), zmq::socket_ref(capture));
            }
            else
            {
                // Run proxy (blocks until context terminated)
                zmq::proxy(zmq::socket_ref(frontend), zmq::socket_ref(backend));
            }
        }
        catch (const zmq::error_t &e)
        {
            // Check if this is due to stop request (context termination)
            if (e.num() != ETERM && !stopRequested_.load())
            {
                handleError(e, "proxy thread error");
            }
        }
        catch (const std::exception &e)
        {
            if (errorCallback_ && !stopRequested_.load())
            {
                errorCallback_(std::string("proxy thread exception: ") + e.what());
            }
        }

        running_ = false;
    }

} // namespace limp
