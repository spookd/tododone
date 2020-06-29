#include "task_list.h"

void InitAll(v8::Local<v8::Object> exports) {
  TaskList::Init(exports);
}

NODE_MODULE(addon, InitAll);