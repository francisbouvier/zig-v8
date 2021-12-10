// Based on https://github.com/denoland/rusty_v8/blob/main/src/binding.cc

#include <cassert>
#include "include/libplatform/libplatform.h"
#include "include/v8.h"

// for PrintF
// v8::internal::PrintF(stdout, "%s", "");
//#include "src/utils/utils.h"

template <class T, class... Args>
class Wrapper {
    public:
        Wrapper(T* buf, Args... args) : inner_(args...) {}
    private:
        T inner_;
};

template <class T, class... Args>
void construct_in_place(T* buf, Args... args) {
    new (buf) Wrapper<T, Args...>(buf, std::forward<Args>(args)...);
}

template <class T>
inline static T* local_to_ptr(v8::Local<T> local) {
    return *local;
}

template <class T>
inline static const v8::Local<T> ptr_to_local(const T* ptr) {
    static_assert(sizeof(v8::Local<T>) == sizeof(T*), "");
    auto local = *reinterpret_cast<const v8::Local<T>*>(&ptr);
    assert(*local == ptr);
    return local;
}

template <class T>
inline static const v8::MaybeLocal<T> ptr_to_maybe_local(const T* ptr) {
    static_assert(sizeof(v8::MaybeLocal<T>) == sizeof(T*), "");
    return *reinterpret_cast<const v8::MaybeLocal<T>*>(&ptr);
}

template <class T>
inline static T* maybe_local_to_ptr(v8::MaybeLocal<T> local) {
    return *local.FromMaybe(v8::Local<T>());
}

extern "C" {

// Platform

v8::Platform* v8__Platform__NewDefaultPlatform(int thread_pool_size, bool idle_task_support) {
    return v8::platform::NewDefaultPlatform(
        thread_pool_size,  
        idle_task_support ? v8::platform::IdleTaskSupport::kEnabled : v8::platform::IdleTaskSupport::kDisabled,
        v8::platform::InProcessStackDumping::kDisabled,
        nullptr
    ).release();
}

void v8__Platform__DELETE(v8::Platform* self) { delete self; }

bool v8__Platform__PumpMessageLoop(v8::Platform* platform, v8::Isolate* isolate, bool wait_for_work) {
    return v8::platform::PumpMessageLoop(
        platform, isolate,
        wait_for_work ? v8::platform::MessageLoopBehavior::kWaitForWork : v8::platform::MessageLoopBehavior::kDoNotWait);
}

// V8

const char* v8__V8__GetVersion() { return v8::V8::GetVersion(); }

void v8__V8__InitializePlatform(v8::Platform* platform) {
    v8::V8::InitializePlatform(platform);
}

void v8__V8__Initialize() { v8::V8::Initialize(); }

int v8__V8__Dispose() { return v8::V8::Dispose(); }

void v8__V8__ShutdownPlatform() { v8::V8::ShutdownPlatform(); }

// ArrayBuffer

v8::ArrayBuffer::Allocator* v8__ArrayBuffer__Allocator__NewDefaultAllocator() {
    return v8::ArrayBuffer::Allocator::NewDefaultAllocator();
}

void v8__ArrayBuffer__Allocator__DELETE(v8::ArrayBuffer::Allocator* self) { delete self; }

// Isolate

v8::Isolate* v8__Isolate__New(const v8::Isolate::CreateParams& params) {
    return v8::Isolate::New(params);
}

void v8__Isolate__Dispose(v8::Isolate* isolate) { isolate->Dispose(); }

void v8__Isolate__Enter(v8::Isolate* isolate) { isolate->Enter(); }

void v8__Isolate__Exit(v8::Isolate* isolate) { isolate->Exit(); }

const v8::Context* v8__Isolate__GetCurrentContext(v8::Isolate* isolate) {
    return local_to_ptr(isolate->GetCurrentContext());
}

size_t v8__Isolate__CreateParams__SIZEOF() {
    return sizeof(v8::Isolate::CreateParams);
}

void v8__Isolate__CreateParams__CONSTRUCT(v8::Isolate::CreateParams* buf) {
    // Use in place new constructor otherwise special fields like shared_ptr will attempt to do copy and fail if the buffer had undefined values.
    new (buf) v8::Isolate::CreateParams();
}

// HandleScope

void v8__HandleScope__CONSTRUCT(v8::HandleScope* buf, v8::Isolate* isolate) {
    // We can't do in place new, since new is overloaded for HandleScope.
    // Use in place construct instead.
    construct_in_place<v8::HandleScope>(buf, isolate);
}

void v8__HandleScope__DESTRUCT(v8::HandleScope* scope) { scope->~HandleScope(); }

// Context

v8::Context* v8__Context__New(v8::Isolate* isolate, const v8::ObjectTemplate* global_tmpl, const v8::Value* global_obj) {
    return local_to_ptr(
        v8::Context::New(isolate, nullptr, ptr_to_maybe_local(global_tmpl), ptr_to_maybe_local(global_obj))
    );
}

void v8__Context__Enter(const v8::Context& context) { ptr_to_local(&context)->Enter(); }

void v8__Context__Exit(const v8::Context& context) { ptr_to_local(&context)->Exit(); }

v8::Isolate* v8__Context__GetIsolate(const v8::Context& self) {
	return ptr_to_local(&self)->GetIsolate();
}

// ScriptOrigin

void v8__ScriptOrigin__CONSTRUCT(v8::ScriptOrigin* buf, v8::Isolate* isolate, const v8::Value& resource_name) {
    new (buf) v8::ScriptOrigin(isolate, ptr_to_local(&resource_name));
}

// Script

v8::Script* v8__Script__Compile(const v8::Context& context, const v8::String& src, const v8::ScriptOrigin& origin) {
    return maybe_local_to_ptr(
        v8::Script::Compile(ptr_to_local(&context), ptr_to_local(&src), const_cast<v8::ScriptOrigin*>(&origin))
    );
}

v8::Value* v8__Script__Run(const v8::Script& script, const v8::Context& context) {
    return maybe_local_to_ptr(ptr_to_local(&script)->Run(ptr_to_local(&context)));
}

// String

v8::String* v8__String__NewFromUtf8(v8::Isolate* isolate, const char* data, v8::NewStringType type, int length) {
    return maybe_local_to_ptr(
        v8::String::NewFromUtf8(isolate, data, type, length)
    );
}

int v8__String__WriteUtf8(const v8::String& str,
                          v8::Isolate* isolate,
                          char* buffer,
                          int length,
                          int* nchars_ref,
                          int options) {
    return str.WriteUtf8(isolate, buffer, length, nchars_ref, options);
}

int v8__String__Utf8Length(const v8::String& self, v8::Isolate* isolate) {
    return self.Utf8Length(isolate);
}

// Value

const v8::String* v8__Value__ToString(const v8::Value& val,
                                      const v8::Context& ctx) {
    return maybe_local_to_ptr(val.ToString(ptr_to_local(&ctx)));
}

// TryCatch

size_t v8__TryCatch__SIZEOF() {
    return sizeof(v8::TryCatch);
}

void v8__TryCatch__CONSTRUCT(v8::TryCatch* buf, v8::Isolate* isolate) {
    construct_in_place<v8::TryCatch>(buf, isolate);
}

void v8__TryCatch__DESTRUCT(v8::TryCatch* self) { self->~TryCatch(); }

const v8::Value* v8__TryCatch__Exception(const v8::TryCatch& self) {
    return local_to_ptr(self.Exception());
}

const v8::Message* v8__TryCatch__Message(const v8::TryCatch& self) {
    return local_to_ptr(self.Message());
}

bool v8__TryCatch__HasCaught(const v8::TryCatch& self) {
    return self.HasCaught();
}

const v8::Value* v8__TryCatch__StackTrace(const v8::TryCatch& self,
                                          const v8::Context& context) {
    return maybe_local_to_ptr(self.StackTrace(ptr_to_local(&context)));
}

// Message

const v8::String* v8__Message__GetSourceLine(const v8::Message& self,
                                             const v8::Context& context) {
    return maybe_local_to_ptr(self.GetSourceLine(ptr_to_local(&context)));
}

const v8::Value* v8__Message__GetScriptResourceName(const v8::Message& self) {
    return local_to_ptr(self.GetScriptResourceName());
}

int v8__Message__GetLineNumber(const v8::Message& self,
                               const v8::Context& context) {
    v8::Maybe<int> maybe = self.GetLineNumber(ptr_to_local(&context));
    return maybe.FromMaybe(-1);
}

int v8__Message__GetStartColumn(const v8::Message& self) {
    return self.GetStartColumn();
}

int v8__Message__GetEndColumn(const v8::Message& self) {
    return self.GetEndColumn();
}

}