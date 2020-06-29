#ifndef __H_TASK_LIST
#define __H_TASK_LIST

#include <string.h>
#include <vector>
#include <nan.h>

typedef struct {
    struct __attribute__((packed)) {
        bool isNew: 1;
        bool isModified: 1;
        bool isDeleted: 1;
        bool isCompleted: 1;
        uint8_t rfu: 4;
    } info;

    // Fixed size for faster and easier lookups. Pitfalls would be
    // larger file sizes and limited text lengths.
    char text[UINT8_MAX];
} task;

class TaskList : public Nan::ObjectWrap {
public:
    static void Init(v8::Local<v8::Object> exports);

private:
    std::vector<task> _tasks;
    std::vector<size_t> _deletedTasks;
    
    explicit TaskList();
    ~TaskList();

    void Load(std::string fileName = nullptr);
    void Save(std::string fileName = nullptr);

    static NAN_METHOD(New);

    static NAN_METHOD(Count);
    static NAN_METHOD(Get);
    static NAN_METHOD(Add);
    static NAN_METHOD(Edit);
    static NAN_METHOD(Delete);

    static NAN_METHOD(Load);
    static NAN_METHOD(Save);

    static Nan::Persistent<v8::Function> constructor;
};

#endif