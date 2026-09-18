#include <memory>
#include <utility>

#include <concepts>
#include <typeinfo>

#include <cstddef>
#include <cstring>

#include <stdexcept>

class bad_any_cast : public std::bad_cast {
public:
    const char* what() const noexcept override {
        return "bad_any_cast: Type mismatch during any_cast";
    }
};

template <std::size_t storage_size = 32, std::size_t alignment = alignof(std::max_align_t)>
class any {
public:
    static_assert(storage_size >= sizeof(void*), "Storage size must be at least pointer size.");
    static_assert(alignment >= alignof(void*), "Alignment must be at least pointer alignment.");

    constexpr any() noexcept : handler_(nullptr) {}

    template <typename T>
        requires (!std::same_as<std::decay_t<T>, any>)
    any(T&& value) {
        emplace<std::decay_t<T>>(std::forward<T>(value));
    }

    any(const any& other) {
        if (other.handler_) {
            other.handler_(operation::clone, &other, this);
        }
    }

    any(any&& other) noexcept {
        if (other.handler_) {
            other.handler_(operation::move, &other, this);
        }
    }

    any& operator=(const any& other) {
        if (this != &other) {
            any(other).swap(*this);
        }
        return *this;
    }

    any& operator=(any&& other) noexcept {
        if (this != &other) {
            reset();
            if (other.handler_) {
                other.handler_(operation::move, &other, this);
            }
        }
        return *this;
    }

    ~any() {
        reset();
    }

    void reset() noexcept {
        if (handler_) {
            handler_(operation::destroy, this, nullptr);
            handler_ = nullptr;
        }
    }

    [[nodiscard]] bool has_value() const noexcept {
        return handler_ != nullptr;
    }

    [[nodiscard]] const std::type_info& type() const noexcept {
        if (!has_value()) return typeid(void);
        return *static_cast<const std::type_info*>(handler_(operation::get_type, this, nullptr));
    }

    void swap(any& other) noexcept {
        if (this == &other) return;

        any temp;
        if (other.handler_) {
            other.handler_(operation::move, &other, &temp);
        }
        if (this->handler_) {
            this->handler_(operation::move, this, &other);
        }
        if (temp.handler_) {
            temp.handler_(operation::move, &temp, this);
        }
    }

    template <typename T, typename... Args>
    std::decay_t<T>& emplace(Args&&... args) {
        reset();
        using value_type = std::decay_t<T>;
        using storage_traits = storage_manager<value_type>;

        storage_traits::construct(storage_buffer_, std::forward<Args>(args)...);
        handler_ = &storage_traits::manage;
        return *storage_traits::get_pointer(storage_buffer_);
    }

    template <typename T>
    friend T* any_cast(any* operand) noexcept;

    template <typename T>
    friend const T* any_cast(const any* operand) noexcept;

private:
    enum class operation { 
        destroy, 
        clone, 
        move, 
        get_type, 
        get_pointer 
    };

    using handler_func = void* (*)(operation op, const any* self, any* target);

    template <typename T>
    static constexpr bool fits_in_soo = 
        (sizeof(T) <= storage_size) && 
        (alignof(T) <= alignment) && 
        std::is_nothrow_move_constructible_v<T>;

    template <typename T>
    struct inline_storage {
        template <typename... Args>
        static void construct(alignas(alignment) std::byte* buffer, Args&&... args) {
            std::construct_at(reinterpret_cast<T*>(buffer), std::forward<Args>(args)...);
        }

        static T* get_pointer(alignas(alignment) std::byte* buffer) noexcept {
            return std::launder(reinterpret_cast<T*>(buffer));
        }

        static const T* get_pointer(const alignas(alignment) std::byte* buffer) noexcept {
            return std::launder(reinterpret_cast<const T*>(buffer));
        }

        static void* manage(operation op, const any* self, any* target) {
            auto self_ptr = get_pointer(const_cast<std::byte*>(self->storage_buffer_));
            switch (op) {
                case operation::destroy:
                    std::destroy_at(self_ptr);
                    return nullptr;
                case operation::clone:
                    std::construct_at(reinterpret_cast<T*>(target->storage_buffer_), *self_ptr);
                    target->handler_ = self->handler_;
                    return nullptr;
                case operation::move:
                    std::construct_at(reinterpret_cast<T*>(target->storage_buffer_), std::move(*self_ptr));
                    target->handler_ = self->handler_;
                    std::destroy_at(self_ptr);
                    const_cast<any*>(self)->handler_ = nullptr;
                    return nullptr;
                case operation::get_type:
                    return const_cast<void*>(static_cast<const void*>(&typeid(T)));
                case operation::get_pointer:
                    return const_cast<T*>(self_ptr);
            }
            return nullptr;
        }
    };

    template <typename T>
    struct heap_storage {
        template <typename... Args>
        static void construct(alignas(alignment) std::byte* buffer, Args&&... args) {
            T* heap_ptr = new T(std::forward<Args>(args)...);
            std::memcpy(buffer, &heap_ptr, sizeof(T*));
        }

        static T* get_pointer(const alignas(alignment) std::byte* buffer) noexcept {
            T* heap_ptr;
            std::memcpy(&heap_ptr, buffer, sizeof(T*));
            return heap_ptr;
        }

        static void* manage(operation op, const any* self, any* target) {
            T* self_ptr = get_pointer(self->storage_buffer_);
            switch (op) {
                case operation::destroy:
                    delete self_ptr;
                    return nullptr;
                case operation::clone:
                    heap_storage<T>::construct(target->storage_buffer_, *self_ptr);
                    target->handler_ = self->handler_;
                    return nullptr;
                case operation::move:
                    std::memcpy(target->storage_buffer_, self->storage_buffer_, sizeof(T*));
                    target->handler_ = self->handler_;
                    const_cast<any*>(self)->handler_ = nullptr;
                    return nullptr;
                case operation::get_type:
                    return const_cast<void*>(static_cast<const void*>(&typeid(T)));
                case operation::get_pointer:
                    return self_ptr;
            }
            return nullptr;
        }
    };

    template <typename T>
    using storage_manager = std::conditional_t<fits_in_soo<T>, inline_storage<T>, heap_storage<T>>;

    alignas(alignment) mutable std::byte storage_buffer_[storage_size];
    handler_func handler_ = nullptr;
};

template <typename T>
T* any_cast(any* operand) noexcept {
    if (operand && operand->type() == typeid(T)) {
        return static_cast<T*>(operand->handler_(any::operation::get_pointer, operand, nullptr));
    }
    return nullptr;
}

template <typename T>
const T* any_cast(const any* operand) noexcept {
    return any_cast<T>(const_cast<any*>(operand));
}

template <typename T>
T any_cast(any& operand) {
    using non_ref_type = std::remove_reference_t<T>;
    auto* result = any_cast<non_ref_type>(&operand);
    if (!result) throw bad_any_cast();
    return static_cast<T>(*result);
}

template <typename T>
T any_cast(const any& operand) {
    using non_ref_type = std::remove_reference_t<T>;
    auto* result = any_cast<non_ref_type>(&operand);
    if (!result) throw bad_any_cast();
    return static_cast<T>(*result);
}
