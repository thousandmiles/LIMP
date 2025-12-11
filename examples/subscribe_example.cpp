#include <limp/limp.hpp>
#include <iostream>
#include <map>
#include <set>

using namespace limp;

// Simple subscription manager
class SubscriptionManager
{
public:
    struct Subscription
    {
        uint16_t subscriberNode;
        uint16_t classID;
        uint16_t instanceID;
        uint16_t attrID;

        bool operator<(const Subscription &other) const
        {
            if (classID != other.classID)
                return classID < other.classID;
            if (instanceID != other.instanceID)
                return instanceID < other.instanceID;
            if (attrID != other.attrID)
                return attrID < other.attrID;
            return subscriberNode < other.subscriberNode;
        }
    };

    void addSubscription(const Frame &subFrame)
    {
        Subscription sub{
            subFrame.srcNodeID,
            subFrame.classID,
            subFrame.instanceID,
            subFrame.attrID};
        subscriptions_.insert(sub);
        std::cout << "Added subscription: Node 0x" << std::hex << sub.subscriberNode
                  << " -> Class 0x" << sub.classID
                  << ", Instance " << std::dec << sub.instanceID
                  << ", Attr " << sub.attrID << "\n";
    }

    void removeSubscription(const Frame &unsubFrame)
    {
        Subscription sub{
            unsubFrame.srcNodeID,
            unsubFrame.classID,
            unsubFrame.instanceID,
            unsubFrame.attrID};
        subscriptions_.erase(sub);
        std::cout << "Removed subscription: Node 0x" << std::hex << sub.subscriberNode
                  << " -> Class 0x" << sub.classID
                  << ", Instance " << std::dec << sub.instanceID
                  << ", Attr " << sub.attrID << "\n";
    }

    std::vector<uint16_t> getSubscribers(uint16_t classID, uint16_t instanceID, uint16_t attrID) const
    {
        std::vector<uint16_t> subscribers;
        for (const auto &sub : subscriptions_)
        {
            if (sub.classID == classID &&
                sub.instanceID == instanceID &&
                sub.attrID == attrID)
            {
                subscribers.push_back(sub.subscriberNode);
            }
        }
        return subscribers;
    }

    size_t count() const { return subscriptions_.size(); }

private:
    std::set<Subscription> subscriptions_;
};

// Simulate tag value storage
class TagSystem
{
public:
    void setValue(uint16_t instanceID, float value)
    {
        tags_[instanceID] = value;
    }

    std::optional<float> getValue(uint16_t instanceID) const
    {
        auto it = tags_.find(instanceID);
        if (it != tags_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::map<uint16_t, float> tags_;
};

int main()
{
    std::cout << "=== LIMP Subscribe/Unsubscribe Example ===\n\n";

    SubscriptionManager subManager;
    TagSystem tagSystem;

    // Initialize some tag values
    tagSystem.setValue(7, 100.0f);
    tagSystem.setValue(8, 200.0f);

    // Example 1: HMI subscribes to Tag[7].Value
    std::cout << "1. HMI subscribes to Tag[7].Value\n";
    std::cout << "-----------------------------------\n";

    auto subscribe1 = MessageBuilder::subscribe(
                          static_cast<uint16_t>(NodeID::HMI),
                          static_cast<uint16_t>(NodeID::PLC),
                          static_cast<uint16_t>(ClassID::Tag),
                          7,
                          TagAttr::Value)
                          .build();

    subManager.addSubscription(subscribe1);
    std::cout << "Total subscriptions: " << subManager.count() << "\n\n";

    // Example 2: Server subscribes to Tag[7].Value
    std::cout << "2. Server subscribes to Tag[7].Value\n";
    std::cout << "-------------------------------------\n";

    auto subscribe2 = MessageBuilder::subscribe(
                          static_cast<uint16_t>(NodeID::Server),
                          static_cast<uint16_t>(NodeID::PLC),
                          static_cast<uint16_t>(ClassID::Tag),
                          7,
                          TagAttr::Value)
                          .build();

    subManager.addSubscription(subscribe2);
    std::cout << "Total subscriptions: " << subManager.count() << "\n\n";

    // Example 3: HMI subscribes to Tag[8].Value
    std::cout << "3. HMI subscribes to Tag[8].Value\n";
    std::cout << "----------------------------------\n";

    auto subscribe3 = MessageBuilder::subscribe(
                          static_cast<uint16_t>(NodeID::HMI),
                          static_cast<uint16_t>(NodeID::PLC),
                          static_cast<uint16_t>(ClassID::Tag),
                          8,
                          TagAttr::Value)
                          .build();

    subManager.addSubscription(subscribe3);
    std::cout << "Total subscriptions: " << subManager.count() << "\n\n";

    // Example 4: Simulate Tag[7] value change - send EVENT to subscribers
    std::cout << "4. Tag[7] value changed: 100.0 -> 123.45\n";
    std::cout << "-----------------------------------------\n";

    tagSystem.setValue(7, 123.45f);

    auto subscribers = subManager.getSubscribers(
        static_cast<uint16_t>(ClassID::Tag), 7, TagAttr::Value);

    std::cout << "Sending EVENT to " << subscribers.size() << " subscriber(s):\n";

    for (auto subscriberNode : subscribers)
    {
        auto event = MessageBuilder::event(
                         static_cast<uint16_t>(NodeID::PLC),
                         subscriberNode,
                         static_cast<uint16_t>(ClassID::Tag),
                         7,
                         TagAttr::Value)
                         .setPayload(123.45f)
                         .enableCRC()
                         .build();

        MessageParser parser(event);
        std::cout << "  -> Node 0x" << std::hex << subscriberNode << std::dec;
        if (auto val = parser.getFloat32())
        {
            std::cout << ", Value: " << *val << "\n";
        }
    }
    std::cout << "\n";

    // Example 5: HMI unsubscribes from Tag[7].Value
    std::cout << "5. HMI unsubscribes from Tag[7].Value\n";
    std::cout << "--------------------------------------\n";

    auto unsubscribe = MessageBuilder::unsubscribe(
                           static_cast<uint16_t>(NodeID::HMI),
                           static_cast<uint16_t>(NodeID::PLC),
                           static_cast<uint16_t>(ClassID::Tag),
                           7,
                           TagAttr::Value)
                           .build();

    subManager.removeSubscription(unsubscribe);
    std::cout << "Total subscriptions: " << subManager.count() << "\n\n";

    // Example 6: Tag[7] changes again - only Server gets event now
    std::cout << "6. Tag[7] value changed again: 123.45 -> 200.0\n";
    std::cout << "----------------------------------------------\n";

    tagSystem.setValue(7, 200.0f);

    subscribers = subManager.getSubscribers(
        static_cast<uint16_t>(ClassID::Tag), 7, TagAttr::Value);

    std::cout << "Sending EVENT to " << subscribers.size() << " subscriber(s):\n";

    for (auto subscriberNode : subscribers)
    {
        auto event = MessageBuilder::event(
                         static_cast<uint16_t>(NodeID::PLC),
                         subscriberNode,
                         static_cast<uint16_t>(ClassID::Tag),
                         7,
                         TagAttr::Value)
                         .setPayload(200.0f)
                         .enableCRC()
                         .build();

        MessageParser parser(event);
        std::cout << "  -> Node 0x" << std::hex << subscriberNode << std::dec;
        if (auto val = parser.getFloat32())
        {
            std::cout << ", Value: " << *val << "\n";
        }
    }

    std::cout << "\n=== Subscribe Example Complete ===\n";

    return 0;
}
