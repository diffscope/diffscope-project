#ifndef DIFFSCOPE_COREPLUGIN_PROPERTYMAPPERDATA_P_H
#define DIFFSCOPE_COREPLUGIN_PROPERTYMAPPERDATA_P_H

#include <QObject>
#include <QVariant>

namespace Core {

    template <class T, class O, O *(T::*f)() const>
    struct PropertyObjectMemberGetterTag {
        static constexpr auto propertyObjectMemberGetter = f;
    };

    template <class T, typename Pg, typename Ps, class O = T>
    struct PropertyMetadata {

        using Property = Pg;
        using Getter = Pg(O::*)() const;
        using Setter = void(O::*)(Ps);
        using NotifySignal = void(O::*)(Ps);

        Getter getter;
        Setter setter;
        NotifySignal notifySignal;
        O *(*propertyObjectGetter)(const T *) = [](const T *t) { return const_cast<T *>(t); };

        constexpr PropertyMetadata(Getter getter, Setter setter, NotifySignal notifySignal)
            : getter(getter), setter(setter), notifySignal(notifySignal) {}

        constexpr PropertyMetadata(Getter getter, Setter setter, NotifySignal notifySignal, O *(*propertyObjectGetter)(T *))
            : getter(getter), setter(setter), notifySignal(notifySignal), propertyObjectGetter(propertyObjectGetter) {}

        template <typename OMG>
        constexpr PropertyMetadata(Getter getter, Setter setter, NotifySignal notifySignal, OMG)
            : getter(getter), setter(setter), notifySignal(notifySignal), propertyObjectGetter([](const T *t) { return (t->*OMG::propertyObjectMemberGetter)(); }) {}
    };

    template <
        class PublicClass,
        class PrivateClass,
        class T,
        typename ...SP
    >
    class PropertyMapperData {
        static constexpr auto N = sizeof...(SP);

        template<int propertyIndex>
        using PropertyType = typename std::tuple_element_t<propertyIndex, std::tuple<SP...>>::Property;

        template <template <typename> typename Container, typename IndexSequence>
        struct TupleMaker;

        template <template <typename> typename Container, size_t... Is>
        struct TupleMaker<Container, std::index_sequence<Is...>> {
            using type = std::tuple<Container<PropertyType<Is>>...>;
        };

        template<typename P>
        using PropertyToItemsContainer = QHash<P, QSet<T *>>;

        template<typename P>
        using ItemToPropertyContainer = QHash<T *, P>;

        template <template <typename> typename Container>
        using ContainerTuple = typename TupleMaker<Container, std::make_index_sequence<N>>::type;

        ContainerTuple<PropertyToItemsContainer> propertyToItems;
        ContainerTuple<ItemToPropertyContainer> itemToProperty;
        QVariant cache[N];
        QSet<T *> selectedItems;

    public:
        PublicClass *q_ptr = nullptr;
        std::tuple<SP...> propertyMetadataList;

        PropertyMapperData(SP ...propertyMetadataList)
            : propertyMetadataList(std::make_tuple(propertyMetadataList...)) {
        }

        void handleItemSelected(T* item, bool selected) {
            if (selected) {
                addItem(item);
            } else {
                removeItem(item);
            }
            refreshCache();
        }

        void addItem(T *item) {
            auto q = q_ptr;
            updateAllProperties(item);
            connectAllProperties(item);
            QObject::connect(item, &QObject::destroyed, q, [item, this] {
                removeItem<false>(item);
                refreshCache();
            });
            selectedItems.insert(item);
        }

        template<int propertyIndex>
        void disconnectProperty(T *item) {
            auto q = q_ptr;
            const auto notifySignal = std::get<propertyIndex>(propertyMetadataList).notifySignal;
            auto o = std::get<propertyIndex>(propertyMetadataList).propertyObjectGetter(item);
            QObject::disconnect(o, notifySignal, q, nullptr);
        }

        template<size_t... i>
        void disconnectAllPropertiesImpl(T *item, std::index_sequence<i...>) {
            (disconnectProperty<i>(item), ...);
        }

        template<int i>
        void removeItemImpl_i(T *item) {
            auto &itemToProperty_i = std::get<i>(itemToProperty);
            auto &propertyToItems_i = std::get<i>(propertyToItems);
            const auto oldValue = itemToProperty_i.value(item);
            propertyToItems_i[oldValue].remove(item);
            if (propertyToItems_i[oldValue].isEmpty()) {
                propertyToItems_i.remove(oldValue);
            }
            itemToProperty_i.remove(item);
        }

        template<size_t... i>
        void removeItemImpl(T *item, std::index_sequence<i...>) {
            (removeItemImpl_i<i>(item), ...);
        }

        template <bool shouldDisconnectProperty = true>
        void removeItem(T *item) {
            auto q = q_ptr;
            if constexpr (shouldDisconnectProperty) {
                disconnectAllPropertiesImpl(item, std::make_index_sequence<N>{});
            }
            removeItemImpl(item, std::make_index_sequence<N>{});
            selectedItems.remove(item);
        }

        template<int i>
        void clearImpl_i() {
            std::get<i>(propertyToItems).clear();
            std::get<i>(itemToProperty).clear();
            cache[i].clear();
        }

        template<size_t... i>
        void clearImpl(std::index_sequence<i...>) {
            (clearImpl_i<i>(), ...);
        }

        void clear() {
            for (auto *item : selectedItems) {
                disconnectAllPropertiesImpl(item, std::make_index_sequence<N>{});
            }
            selectedItems.clear();
            clearImpl(std::make_index_sequence<N>{});
        }

        template <int propertyIndex>
        void updateProperty(T *item, auto value) {
            auto &propertyToItems_i = std::get<propertyIndex>(propertyToItems);
            auto &itemToProperty_i = std::get<propertyIndex>(itemToProperty);
            if (itemToProperty_i.contains(item)) {
                const auto oldValue = itemToProperty_i.value(item);
                if (oldValue == value) {
                    return;
                }
                propertyToItems_i[oldValue].remove(item);
                if (propertyToItems_i[oldValue].isEmpty()) {
                    propertyToItems_i.remove(oldValue);
                }
            }
            itemToProperty_i.insert(item, value);
            propertyToItems_i[value].insert(item);
            refreshCacheImpl_i<propertyIndex>();
        }

        template<size_t... i>
        void updateAllPropertiesImpl(T* item, std::index_sequence<i...>) {
            (updateProperty<i>(item, (std::get<i>(propertyMetadataList).propertyObjectGetter(item)->*std::get<i>(propertyMetadataList).getter)()), ...);
        }

        void updateAllProperties(T *item) {
            updateAllPropertiesImpl(item, std::make_index_sequence<N>{});
        }

        template <int propertyIndex>
        void connectProperty(T *item) {
            auto q = q_ptr;
            const auto notifySignal = std::get<propertyIndex>(propertyMetadataList).notifySignal;
            auto o = std::get<propertyIndex>(propertyMetadataList).propertyObjectGetter(item);
            QObject::connect(o, notifySignal, q, [item, this](auto v) {
                updateProperty<propertyIndex>(item, v);
            });
        }

        template<size_t... i>
        void connectAllPropertiesImpl(T* item, std::index_sequence<i...>) {
            (connectProperty<i>(item), ...);
        }

        void connectAllProperties(T *item) {
            connectAllPropertiesImpl(item, std::make_index_sequence<N>{});
        }

        template <int propertyIndex>
        QVariant unifiedProperty() const {
            auto &propertyToItems_i = std::get<propertyIndex>(propertyToItems);
            auto &itemToProperty_i = std::get<propertyIndex>(itemToProperty);
            const int count = itemToProperty_i.size();
            if (count == 0 || propertyToItems_i.size() != 1) {
                return {};
            }
            const auto it = propertyToItems_i.constBegin();
            if (it.value().size() != count) {
                return {};
            }
            return it.key();
        }

        template<int i>
        void refreshCacheImpl_i() {
            auto q = q_ptr;
            const QVariant newValue = unifiedProperty<i>();
            if (cache[i] != newValue) {
                cache[i] = newValue;
                static_cast<PrivateClass *>(this)->template notifyValueChange<i>();
            }
        }

        template<size_t... i>
        void refreshCacheImpl(std::index_sequence<i...>) {
            (refreshCacheImpl_i<i>(), ...);
        }

        void refreshCache() {
            refreshCacheImpl(std::make_index_sequence<N>{});
        }

        template <int propertyIndex>
        QVariant value() const {
            return cache[propertyIndex];
        }

        template <int propertyIndex>
        void setValue(const QVariant &value) {
            const auto setter = std::get<propertyIndex>(propertyMetadataList).setter;
            for (auto item : selectedItems) {
                auto o = std::get<propertyIndex>(propertyMetadataList).propertyObjectGetter(item);
                (o->*setter)(value.value<PropertyType<propertyIndex>>());
            }
        }
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROPERTYMAPPERDATA_P_H
