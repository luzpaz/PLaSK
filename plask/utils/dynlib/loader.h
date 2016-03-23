#ifndef PLASK__UTILS_DYNLIB_LOADER_H
#define PLASK__UTILS_DYNLIB_LOADER_H

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#   define PLASK__UTILS_PLUGIN_WINAPI
#   include "../minimal_windows.h"
#else
#   define PLASK__UTILS_PLUGIN_DLOPEN
#   include <dlfcn.h>
#endif

#include <string>
#include <functional>   //std::hash

#include <plask/config.h>

namespace plask {

/**
 * Hold opened shared library. Portable, thin wrapper over system handler to library.
 *
 * Close holded library in destructor.
 */
struct PLASK_API DynamicLibrary {

    enum Flags {
        DONT_CLOSE = 1  ///< if this flag is set DynamicLibrary will not close the library, but it will be closed on application exit
    };

    /// Type of system shared library handler.
#ifdef PLASK__UTILS_PLUGIN_WINAPI
    typedef HINSTANCE handler_t;
#else
    typedef void* handler_t;
#endif

private:
    /// System shared library handler
    handler_t handler;
#ifdef PLASK__UTILS_PLUGIN_WINAPI
    bool unload;    // true if lib. should be unloaded, destructor just don't call FreeLibrary if this is false, undefined when handler == 0
#endif

public:

    static constexpr const char* DEFAULT_EXTENSION =
#ifdef PLASK__UTILS_PLUGIN_WINAPI
            ".dll";
#else
            ".so";
#endif

    /**
     * Open library from file with given name @p filename.
     * @param filename name of file with library to load
     * @param flags flags which describes configuration of open/close process, one or more (or-ed) flags from DynamicLibrary::Flags set
     */
    explicit DynamicLibrary(const std::string& filename, unsigned flags);

    /**
     * Don't open any library. You can call open later.
     */
    DynamicLibrary();

    /// Coping of libary is not alowed
    DynamicLibrary(const DynamicLibrary&) = delete;

    /// Coping of libary is not alowed
    DynamicLibrary& operator=(const DynamicLibrary &) = delete;

    /**
     * Move library ownership from @p to_move to this.
     * @param to_move source of moving library ownership
     */
    DynamicLibrary(DynamicLibrary&& to_move) noexcept;

    /**
     * Move library ownership from @p to_move to this.
     *
     * Same as: <code>swap(to_move);</code>
     * @param to_move source of moving library ownership
     * @return *this
     */
    DynamicLibrary& operator=(DynamicLibrary && to_move) noexcept {
        swap(to_move);  // destructor of to_move will close current this library
        return *this;
    }

    /**
     * Swap library ownerships between this and to_swap.
     * @param to_swap library to swap with
     */
    void swap(DynamicLibrary& to_swap) noexcept {
        std::swap(handler, to_swap.handler);
#ifdef PLASK__UTILS_PLUGIN_WINAPI
        std::swap(unload, to_swap.unload);
#endif
    }

    /**
     * Dispose library.
     */
    ~DynamicLibrary();

    /**
     * Open library from file with given name @p filename.
     *
     * Close already opened library wrapped by this if any.
     * @param filename name of file with library to load
     * @param flags flags which describe configuration of open/close process, one or more (or-ed) flags from DynamicLibrary::Flags set
     */
    void open(const std::string& filename, unsigned flags);

    /**
     * Close opened library.
     */
    void close();

    /**
     * Get symbol from library.
     *
     * Throw excpetion if library is not opened.
     * @param symbol_name name of symbol to get
     * @return symbol with given name, or @c nullptr if there is no symbol with given name
     */
    void* getSymbol(const std::string &symbol_name) const;

    /**
     * Get symbol from library and cast it to given type.
     *
     * Throw excpetion if library is not opened.
     * @param symbol_name name of symbol to get
     * @return symbol with given name casted to given type, or @c nullptr if there is no symbol with given name
     * @tparam SymbolType required type to which symbol will be casted
     */
    template <typename SymbolType>
    SymbolType getSymbol(const std::string &symbol_name) const {
        return reinterpret_cast<SymbolType>(getSymbol(symbol_name));
    }

    /// Same as getSymbol(const std::string &symbol_name)
    void* operator[](const std::string &symbol_name) const {
        return getSymbol(symbol_name);
    }

    /**
     * Get symbol from library.
     *
     * Throw excpetion if library is not opened or if there is no symbol with given name.
     * @param symbol_name name of symbol to get
     * @return symbol with given name
     */
    void* requireSymbol(const std::string &symbol_name) const;

    /**
     * Get symbol from library and cast it to given type.
     *
     * Throw excpetion if library is not opened or if there is no symbol with given name.
     * @param symbol_name name of symbol to get
     * @return symbol with given name, casted to given type
     * @tparam SymbolType required type to which symbol will be casted
     */
    template <typename SymbolType>
    SymbolType requireSymbol(const std::string &symbol_name) const {
        return reinterpret_cast<SymbolType>(requireSymbol(symbol_name));
    }

    /**
     * Check if library is already open.
     * @return @c true only if library is already open
     */
    bool isOpen() const { return handler != 0; }

    /**
     * Get system handler.
     *
     * Type of result is system specyfic (DynamicLibrary::handler_t).
     * @return system handler
     */
    handler_t getSystemHandler() const { return handler; }

    /**
     * Release ownership over holded system library handler.
     * This does not close the library.
     * @return system library handler which ownership has been relased
     */
    handler_t release();

    /**
     * Compare operator, defined to allow store dynamic libriaries in standard containers which require this.
     */
    bool operator<(const DynamicLibrary& other) const {
#ifdef PLASK__UTILS_PLUGIN_WINAPI
        return this->handler < other.handler || (this->handler == other.handler && !this->unload && other.unload);
#else
        return this->handler < other.handler;
#endif
    }

    /**
     * Compare operator, defined to allow store dynamic libriaries in standard containers which require this.
     */
    bool operator == (const DynamicLibrary& other) const {
#ifdef PLASK__UTILS_PLUGIN_WINAPI
        return (this->handler == other.handler) && (this->unload == other.unload);
#else
        return this->handler == other.handler;
#endif
    }

};

}   // namespace plask

namespace std {

/// std::swap implementation for dynamic libraries
inline void swap(plask::DynamicLibrary& a, plask::DynamicLibrary& b) noexcept { a.swap(b); }

/// hash method, allow to store dynamic libraries in hash maps
template<>
struct hash<plask::DynamicLibrary> {
    std::hash<plask::DynamicLibrary::handler_t> h;
public:
    std::size_t operator()(const plask::DynamicLibrary &s) const {
        return h(s.getSystemHandler());
    }
};



}

#endif // PLASK__UTILS_DYNLIB_LOADER_H
