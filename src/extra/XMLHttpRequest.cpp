#include "StarFishConfig.h"
#include "XMLHttpRequest.h"

#include <curl/curl.h>
#include <future>
#include <Elementary.h>

namespace StarFish {

void XMLHttpRequest::send(String* body)
{
      escargot::ESObject* obj = (escargot::ESObject*)this;
      const char* url = m_url->utf8Data();

      auto task  = [](escargot::ESObject* obj,const char* url, XMLHttpRequest::METHOD_TYPE methodType, const char* body)->bool{

      Buffer header;
      header.memory=NULL;
      header.size=0;

      Buffer buffer;
      buffer.memory=NULL;
      buffer.size=0;

      CURL* curl = curl_easy_init();
      CURLcode res;

      if (!curl) return false;

      curl_easy_setopt(curl, CURLOPT_URL,url );
      //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
      curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) &header);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) &buffer);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

      if(methodType==POST_METHOD){
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
      }else if(methodType==GET_METHOD){

      }else{
        printf("XMLHttpRequest UnSupportted method!!  \n");
        return false;
      }
      //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

      res = curl_easy_perform(curl);

      long res_code = 0;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
      if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
      {
          printf("!!! Response code: %ld\n", res_code);
      }
      curl_easy_cleanup(curl);

      if(buffer.size==0)
          return false;

      struct Pass {
          escargot::ESObject* obj;
          char* buf;
          char* header;

      };

      Pass* pass = new Pass;
      pass->buf = buffer.memory;
      pass->header = header.memory;
      pass->obj = obj;

      ecore_thread_main_loop_begin();
      ecore_idler_add([](void *data)->Eina_Bool{
          Pass* pass = (Pass*)data;

          escargot::ESObject* this_obj = pass->obj;
          escargot::ESVMInstance* instance = escargot::ESVMInstance::currentInstance();
          printf("HEADER : %s\n",pass->header);

          switch(((XMLHttpRequest*)this_obj)->getResponseType()){
            case JSON_RESPONSE:
              {
                escargot::ESValue json_arg[1] = {escargot::ESValue(escargot::ESString::create(pass->buf))};
                escargot::ESValue json_parse_fn = instance->globalObject()->json()->get(escargot::ESValue(escargot::ESString::create("parse")));

                std::jmp_buf tryPosition;
                if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
                    escargot::ESValue ret = escargot::ESFunctionObject::call(instance, json_parse_fn, instance->globalObject()->json(), json_arg, 1, false);
                    this_obj->set(escargot::ESString::create("response"),ret);
                    instance->unregisterTryPos(&tryPosition);
                } else {
                    escargot::ESValue err = instance->getCatchedError();
                    printf("Uncaught %s\n", err.toString()->utf8Data());
                }
              }
              break;

            case TEXT_RESPONSE:
            default:
              this_obj->set(escargot::ESString::create("response"),escargot::ESValue(escargot::ESString::create(pass->buf)));
              this_obj->set(escargot::ESString::create("responseText"),escargot::ESValue(escargot::ESString::create(pass->buf)));

          }

          escargot::ESValue fn = ((XMLHttpRequest*)this_obj)->getHandler(String::fromUTF8("load"));
          std::jmp_buf tryPosition;
          if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
              escargot::ESFunctionObject::call(instance, fn, this_obj, NULL, 0, false);
              instance->unregisterTryPos(&tryPosition);
          } else {
              escargot::ESValue err = instance->getCatchedError();
              printf("Uncaught %s\n", err.toString()->utf8Data());
          }
          return ECORE_CALLBACK_CANCEL;
      }, pass);
      ecore_thread_main_loop_end();
      return true;
  };

  //std::future<bool> runTask = std::async(std::launch::async, task);

  std::thread(task,obj,url,m_method,body->utf8Data()).detach();
}


}
