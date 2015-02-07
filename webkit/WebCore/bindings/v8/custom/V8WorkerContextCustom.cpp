

#include "config.h"

#if ENABLE(WORKERS)
#include "V8WorkerContext.h"

#include "DOMTimer.h"
#include "ExceptionCode.h"
#include "ScheduledAction.h"
#include "V8Binding.h"
#include "V8Proxy.h"
#include "V8Utilities.h"
#include "V8WorkerContextEventListener.h"
#include "WebSocket.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"

namespace WebCore {

v8::Handle<v8::Value> SetTimeoutOrInterval(const v8::Arguments& args, bool singleShot)
{
    WorkerContext* workerContext = V8WorkerContext::toNative(args.Holder());

    int argumentCount = args.Length();
    if (argumentCount < 1)
        return v8::Undefined();

    v8::Handle<v8::Value> function = args[0];
    int32_t timeout = argumentCount >= 2 ? args[1]->Int32Value() : 0;
    int timerId;

    v8::Handle<v8::Context> v8Context = workerContext->script()->proxy()->context();
    if (function->IsString()) {
        WebCore::String stringFunction = toWebCoreString(function);
        timerId = DOMTimer::install(workerContext, new ScheduledAction(v8Context, stringFunction, workerContext->url()), timeout, singleShot);
    } else if (function->IsFunction()) {
        size_t paramCount = argumentCount >= 2 ? argumentCount - 2 : 0;
        v8::Local<v8::Value>* params = 0;
        if (paramCount > 0) {
            params = new v8::Local<v8::Value>[paramCount];
            for (size_t i = 0; i < paramCount; ++i)
                params[i] = args[i+2];
        }
        // ScheduledAction takes ownership of actual params and releases them in its destructor.
        ScheduledAction* action = new ScheduledAction(v8Context, v8::Handle<v8::Function>::Cast(function), paramCount, params);
        delete [] params;
        timerId = DOMTimer::install(workerContext, action, timeout, singleShot);
    } else
        return v8::Undefined();

    return v8::Integer::New(timerId);
}

v8::Handle<v8::Value> V8WorkerContext::importScriptsCallback(const v8::Arguments& args)
{
    INC_STATS(L"DOM.WorkerContext.importScripts()");
    if (!args.Length())
        return v8::Undefined();

    String callerURL;
    if (!V8Proxy::sourceName(callerURL))
        return v8::Undefined();
    int callerLine;
    if (!V8Proxy::sourceLineNumber(callerLine))
        return v8::Undefined();
    callerLine += 1;

    Vector<String> urls;
    for (int i = 0; i < args.Length(); i++) {
        v8::TryCatch tryCatch;
        v8::Handle<v8::String> scriptUrl = args[i]->ToString();
        if (tryCatch.HasCaught() || scriptUrl.IsEmpty())
            return v8::Undefined();
        urls.append(toWebCoreString(scriptUrl));
    }

    WorkerContext* workerContext = V8WorkerContext::toNative(args.Holder());

    ExceptionCode ec = 0;
    workerContext->importScripts(urls, callerURL, callerLine, ec);

    if (ec)
        return throwError(ec);

    return v8::Undefined();
}

v8::Handle<v8::Value> V8WorkerContext::setTimeoutCallback(const v8::Arguments& args)
{
    INC_STATS(L"DOM.WorkerContext.setTimeout()");
    return SetTimeoutOrInterval(args, true);
}

v8::Handle<v8::Value> V8WorkerContext::setIntervalCallback(const v8::Arguments& args)
{
    INC_STATS(L"DOM.WorkerContext.setInterval()");
    return SetTimeoutOrInterval(args, false);
}

v8::Handle<v8::Value> V8WorkerContext::addEventListenerCallback(const v8::Arguments& args)
{
    INC_STATS(L"DOM.WorkerContext.addEventListener()");
    WorkerContext* workerContext = V8WorkerContext::toNative(args.Holder());

    RefPtr<EventListener> listener = V8DOMWrapper::getEventListener(workerContext, args[1], false, ListenerFindOrCreate);
    if (listener) {
        String type = toWebCoreString(args[0]);
        bool useCapture = args[2]->BooleanValue();
        workerContext->addEventListener(type, listener, useCapture);

        createHiddenDependency(args.Holder(), args[1], cacheIndex);
    }
    return v8::Undefined();
}

v8::Handle<v8::Value> V8WorkerContext::removeEventListenerCallback(const v8::Arguments& args)
{
    INC_STATS(L"DOM.WorkerContext.removeEventListener()");
    WorkerContext* workerContext = V8WorkerContext::toNative(args.Holder());

    RefPtr<EventListener> listener = V8DOMWrapper::getEventListener(workerContext, args[1], false, ListenerFindOnly);
    if (listener) {
        String type = toWebCoreString(args[0]);
        bool useCapture = args[2]->BooleanValue();
        workerContext->removeEventListener(type, listener.get(), useCapture);

        removeHiddenDependency(args.Holder(), args[1], cacheIndex);
    }
    return v8::Undefined();
}

v8::Handle<v8::Value> toV8(WorkerContext* impl)
{
    if (!impl)
        return v8::Null();

    v8::Handle<v8::Object> global = impl->script()->proxy()->context()->Global();
    ASSERT(!global.IsEmpty());
    return global;
}

} // namespace WebCore

#endif // ENABLE(WORKERS)