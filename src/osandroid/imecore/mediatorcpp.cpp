#include <pch.h>
#include <inputmodel/InputModel.h>
#include "osandroid.h"

using namespace Ribbon;

class ImeCoreWrapper
{
	std::shared_ptr<IJsonInputModel> m_inputModel;

public:
	ImeCoreWrapper() {
		m_inputModel = FACTORYCREATE(JsonInputModel);
	}
	~ImeCoreWrapper() {}

	void PostTask(const char* request) {
		std::string requestStr(request);
		auto atask = std::make_shared<AsyncTask<bool>>([=]() -> bool {
			m_inputModel->JsonCommand(requestStr.c_str());
			return true;
		});
		m_taskQueue.PushTask(atask);
	}
	void PostQuitTask() {
		m_quit = true;
		auto atask = std::make_shared<AsyncTask<bool>>([=]() -> bool {
			return true;
		});
		m_taskQueue.PushTask(atask);
	}
	std::shared_ptr<IAsyncTask> GetTask() {
		if (m_quit)  return std::shared_ptr<IAsyncTask>(nullptr);
		return m_taskQueue.GetTask();
	}
	std::string CompositionState() {
		if (m_quit) return std::string();
		return m_inputModel->CompositionState();
	}
	std::string CandidateState() {
		if (m_quit) return std::string();
		return m_inputModel->CandidateState();
	}
	std::string KeyboardState() {
		if (m_quit) return std::string();
		return m_inputModel->KeyboardState();
	}
	bool IsInitialLaunch(bool doUpdate = false) {
		return m_inputModel->IsInitialLaunch(doUpdate);
	}
private:
	TaskQueue m_taskQueue;
	bool m_quit = false;
};


// int ImeIsInitialLaunch(int)
extern "C" JNIEXPORT jint JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeIsInitialLaunch(JNIEnv* env, jobject thiz, jlong objPtr, jint doUpdate)
{
	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	return pcore->IsInitialLaunch(doUpdate != 0) ? 1 : 0;
}

// int ImeRequest(String)
extern "C" JNIEXPORT void JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeRequest(JNIEnv* env, jobject thiz, jlong objPtr, jstring argument) {
	const char* requestStr = env->GetStringUTFChars(argument, nullptr);
	auto releaseStr = ScopeExit([&]() { env->ReleaseStringUTFChars(argument, requestStr); });

	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	pcore->PostTask(requestStr);
}

// int ImeQuitRequest()
extern "C" JNIEXPORT void JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeQuitRequest(JNIEnv* env, jobject thiz, jlong objPtr) {
	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	pcore->PostQuitTask();
}

// String ImeGetCompositionState(long imeHandle)
extern "C" JNIEXPORT jstring JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeGetCompositionState(JNIEnv* env, jobject thiz, jlong objPtr) {
	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	return env->NewStringUTF(pcore->CompositionState().c_str());
}

// String ImeGetCandidateState(long imeHandle)
extern "C" JNIEXPORT jstring JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeGetCandidateState(JNIEnv* env, jobject thiz, jlong objPtr) {
	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	return env->NewStringUTF(pcore->CandidateState().c_str());
}

// String ImeGetKeyboardState(long imeHandle)
extern "C" JNIEXPORT jstring JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeGetKeyboardState(JNIEnv* env, jobject thiz, jlong objPtr) {
	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	return env->NewStringUTF(pcore->KeyboardState().c_str());
}

// long ImeCreate()
extern "C" JNIEXPORT jlong JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeCreate(JNIEnv* env, jobject thiz)
{
	return reinterpret_cast<jlong>(new ImeCoreWrapper());
}

// void ImeEvent(long)
extern "C" JNIEXPORT void JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeEvent(JNIEnv* env, jobject thiz, jlong objPtr)
{
	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	std::shared_ptr<IAsyncTask> task = pcore->GetTask();
	task->Run();
}

// void ImeDestroy(long)
extern "C" JNIEXPORT void JNICALL
Java_net_millimo_android_ribbon_MediatorJava_ImeDestroy(JNIEnv* env, jobject thiz, jlong objPtr)
{
	ImeCoreWrapper* pcore = reinterpret_cast<ImeCoreWrapper*>(objPtr);
	delete pcore;
}

