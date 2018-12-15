/* DO NOT EDIT THIS FILE - it is machine generated */
#include "jni.h"
/* Header for class com_mogujie_algo_openvino_jni_MoguOpenvino */

#ifndef _Included_com_mogujie_algo_openvino_jni_MoguOpenvino
#define _Included_com_mogujie_algo_openvino_jni_MoguOpenvino
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_mogujie_algo_openvino_jni_MoguOpenvino
 * Method:    create
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_mogujie_algo_openvino_jni_MoguOpenvino_create
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     com_mogujie_algo_openvino_jni_MoguOpenvino
 * Method:    inference
 * Signature: (Ljava/lang/String;[CIII)[F
 */
JNIEXPORT jfloatArray JNICALL Java_com_mogujie_algo_openvino_jni_MoguOpenvino_inference
  (JNIEnv *, jclass, jstring, jcharArray, jint, jint, jint);

/*
 * Class:     com_mogujie_algo_openvino_jni_MoguOpenvino
 * Method:    release
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_mogujie_algo_openvino_jni_MoguOpenvino_release
  (JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif
#endif