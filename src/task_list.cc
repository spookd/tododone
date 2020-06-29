#include "task_list.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <stdlib.h>
#include <memory.h>

/**
 * @brief  Resets the memory used by a vector and clears it.
 * @param  vec: The vector to clear.
 * @retval None
 */
template<typename T> void empty_vector(std::vector<T>& vec) {
    memset(vec.data(), 0, sizeof(T) * vec.size());
    std::vector<T>().swap(vec);
}

Nan::Persistent<v8::Function> TaskList::constructor;

/**
 * @brief  Adds this class/object to an exported object.
 * @param  exports: The v8 Object to add this to.
 * @retval None
 */
void TaskList::Init(v8::Local<v8::Object> exports) {
    v8::Local<v8::Context> context = exports->CreationContext();
    Nan::HandleScope scope;

    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("List").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Set prototype methods
    Nan::SetPrototypeMethod(tpl, "count", Count);
    Nan::SetPrototypeMethod(tpl, "get", Get);
    Nan::SetPrototypeMethod(tpl, "add", Add);
    Nan::SetPrototypeMethod(tpl, "edit", Edit);
    Nan::SetPrototypeMethod(tpl, "delete", Delete);
    Nan::SetPrototypeMethod(tpl, "load", Load);
    Nan::SetPrototypeMethod(tpl, "save", Save);

    // Reset
    constructor.Reset(tpl->GetFunction(context).ToLocalChecked());

    // Add to exports under key "List"
    exports->Set(
        context,
        Nan::New("List").ToLocalChecked(),
        tpl->GetFunction(context).ToLocalChecked()
    );
}

#pragma region NAN

/**
 * @brief  This function called from v8 whenever we're told to initialize a new
 *         TaskList instance.
 * @retval A new instance of TaskList.
 */
NAN_METHOD(TaskList::New) {
    v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

    // Check if this is invoked as a constructor, and if not, make sure it is.
    if (info.IsConstructCall()) {
        TaskList* list = new TaskList();

        list->Wrap(info.This());

        info.GetReturnValue().Set(info.This());
    } else {
        const int argc = 1;

        v8::Local<v8::Value> argv[argc] = { info[0] };
        v8::Local<v8::Function> cons = Nan::New<v8::Function>(constructor);

        info.GetReturnValue().Set(cons->NewInstance(context, argc, argv).ToLocalChecked());
    }
}

/**
 * @brief  Gets the number of tasks currently in our list.
 * @note   This function called from v8 whenever list.count() is invoked in JavaScript.
 * @retval Returns the number of tasks in the list.
 */
NAN_METHOD(TaskList::Count) {
    TaskList* list = ObjectWrap::Unwrap<TaskList>(info.Holder());

    uint32_t numberOfTasks = (uint32_t)(list->_tasks.size() - list->_deletedTasks.size());
    
    info.GetReturnValue().Set(Nan::New<v8::Integer>(numberOfTasks));
}

/**
 * @brief  Gets the specified tasks from the list.
 * @note   This function called from v8 whenever list.get() is invoked in JavaScript.
 * @retval An array with the tasks specified, if found.
 */
NAN_METHOD(TaskList::Get) {
    TaskList* list = ObjectWrap::Unwrap<TaskList>(info.Holder());

    size_t totalTasks = list->_tasks.size() - list->_deletedTasks.size(), i = 0, n = 0;

    size_t count  = info[0]->IsUndefined() ? totalTasks : info[0]->NumberValue(info.GetIsolate()->GetCurrentContext()).FromJust();
    size_t offset = info[1]->IsUndefined() ? 0 : info[1]->NumberValue(info.GetIsolate()->GetCurrentContext()).FromJust();

    offset = MAX(0, MIN(totalTasks - 1, offset));
    count  = MAX(0, MIN(totalTasks - offset, count));

    v8::Local<v8::Array> array = Nan::New<v8::Array>(count);

    i = offset;
    
    while(n != count && i < totalTasks) {
        size_t idx = i++;
        task *t = &list->_tasks.at(idx);

        if (t->info.isDeleted) continue;

        v8::Local<v8::Object> obj = Nan::New<v8::Object>();

        Nan::Set(obj, (v8::Local<v8::String>)Nan::New("index").ToLocalChecked(), (v8::Local<v8::Value>)Nan::New((uint32_t)(idx)));
        Nan::Set(obj, (v8::Local<v8::String>)Nan::New("text").ToLocalChecked(), (v8::Local<v8::Value>)Nan::New(&t->text[0]).ToLocalChecked());
        Nan::Set(obj, (v8::Local<v8::String>)Nan::New("isCompleted").ToLocalChecked(), (v8::Local<v8::Value>)Nan::New(t->info.isCompleted));

        Nan::Set(array, n++, obj);
    }

    info.GetReturnValue().Set(array);
}

/**
 * @brief  Adds a new task to our list with the text specified.
 * @note   This function called from v8 whenever list.add() is invoked in JavaScript.
 * @retval Returns the index of the newly added task.
 */
NAN_METHOD(TaskList::Add) {
    std::string text(*v8::String::Utf8Value(info.GetIsolate(), info[0]));

    if (info[0]->IsUndefined() || text.size() == 0) {
        return Nan::ThrowTypeError("You must supply a text for the task.");
    } else if (text.size() >= UINT8_MAX) {
        return Nan::ThrowTypeError("Text is too large. Try simplifying it.");
    }

    TaskList* list = ObjectWrap::Unwrap<TaskList>(info.Holder());

    // Prepare our task
    task currentTask = {
        .info.isNew = list->_deletedTasks.size() == 0, // Completely new entry?
        .info.isModified = list->_deletedTasks.size() > 0, // ... or is it reused?
        .info.isCompleted = false,
        .info.isDeleted = false,
    };

    memset(&currentTask.text[0], 0, sizeof(currentTask.text));
    memcpy(&currentTask.text[0], text.data(), text.size());

    size_t idx;

    if (currentTask.info.isModified) {
        idx = list->_deletedTasks.back();
        list->_deletedTasks.pop_back();

        list->_tasks.at(idx) = currentTask;
    } else {
        idx = list->_tasks.size();
        list->_tasks.emplace_back(currentTask);
    }
    
    info.GetReturnValue().Set(Nan::New<v8::Integer>((uint32_t)idx));
}

/**
 * @brief  Updates a given task with a new text and/or completion status.
 * @note   This function called from v8 whenever list.edit() is invoked in JavaScript.
 * @retval Returns a reference to our current object (`this`).
 */
NAN_METHOD(TaskList::Edit) {
    if (info[0]->IsUndefined() || info[1]->IsUndefined()) {
        return Nan::ThrowTypeError("You must supply a task index along with the new text.");
    }

    std::string text(*v8::String::Utf8Value(info.GetIsolate(), info[1]));
    size_t idx = info[0]->IsUndefined() ? 0 : info[0]->NumberValue(info.GetIsolate()->GetCurrentContext()).FromJust();
    bool isCompleted = info[2]->IsUndefined() ? false : info[2]->BooleanValue(info.GetIsolate());
    
    if (text.size() == 0) {
        return Nan::ThrowTypeError("You must supply a valid text for the task.");
    } else if (text.size() >= UINT8_MAX) {
        return Nan::ThrowTypeError("Text is too large. Try simplifying it.");
    }

    TaskList* list = ObjectWrap::Unwrap<TaskList>(info.Holder());

    if (idx >= list->_tasks.size()) {
        return Nan::ThrowTypeError("The index you've supplied isn't valid.");
    }

    task *t = &list->_tasks.at(idx);
    
    t->info.isModified = true;
    t->info.isCompleted = isCompleted;

    memset(&t->text[0], 0, sizeof(t->text));
    memcpy(&t->text[0], text.data(), text.size());

    info.GetReturnValue().Set(info.This());
}

/**
 * @brief  Deletes the specified task.
 * @note   This function called from v8 whenever list.delete() is invoked in JavaScript.
 * @retval Returns a reference to our current object (`this`).
 */
NAN_METHOD(TaskList::Delete) {
    if (info[0]->IsUndefined()) {
        return Nan::ThrowTypeError("You must supply a task index to delete.");
    }

    size_t idx = info[0]->IsUndefined() ? 0 : info[0]->NumberValue(info.GetIsolate()->GetCurrentContext()).FromJust();

    TaskList* list = ObjectWrap::Unwrap<TaskList>(info.Holder());

    if (idx >= list->_tasks.size()) {
        return Nan::ThrowTypeError("The index you've supplied isn't valid.");
    }

    task *t = &list->_tasks.at(idx);
    
    t->info = {
        .isCompleted = false,
        .isDeleted = true,
        .isModified = true,
    };
    memset(&t->text[0], 0, sizeof(t->text));

    list->_deletedTasks.emplace_back(idx);

    info.GetReturnValue().Set(info.This());
}


/**
 * @brief  Reads and loads a list of tasks saved to a file.
 * @note   This function called from v8 whenever list.load() is invoked in JavaScript.
 * @retval Returns a Promise.
 */
NAN_METHOD(TaskList::Load) {
    std::string fileName(*v8::String::Utf8Value(info.GetIsolate(), info[0]));

    if (info[0]->IsUndefined() || fileName.size() == 0) {
        return Nan::ThrowTypeError("Seems you forgot to specify where to load the tasks from. Supply a file name.");
    }
    
    TaskList* list = ObjectWrap::Unwrap<TaskList>(info.Holder());

    list->Load(fileName);

    info.GetReturnValue().Set(info.This());
}

/**
 * @brief  Saves the current tasks to a file.
 * @note   This function called from v8 whenever list.save() is invoked in JavaScript.
 * @retval Returns a Promise.
 */
NAN_METHOD(TaskList::Save) {
    std::string fileName(*v8::String::Utf8Value(info.GetIsolate(), info[0]));

    if (info[0]->IsUndefined() || fileName.size() == 0) {
        return Nan::ThrowTypeError("Seems you forgot to specify where to save the tasks. Supply a file name.");
    }
    
    TaskList* list = ObjectWrap::Unwrap<TaskList>(info.Holder());

    list->Save(fileName);

    info.GetReturnValue().Set(info.This());
}

#pragma endregion

/**
 * @brief  Initializes a new TaskList.
 * @retval 
 */
TaskList::TaskList() { }

/**
 * @brief  The destructor of our TaskList. This is where we'd clear memory and clean up things.
 * @retval 
 */
TaskList::~TaskList() { 
    empty_vector(this->_tasks);
    empty_vector(this->_deletedTasks);
}

/**
 * @brief  Loads a list of tasks from a file.
 * @param  fileName: The file of which to load them from.
 * @retval None
 */
void TaskList::Load(std::string fileName) {
    // Clear old tasks
    empty_vector(this->_tasks);

    // Prepare our read stream
    std::ifstream stream(fileName, std::fstream::ate | std::fstream::binary);

    if (stream.good()) {
        size_t totalSize = stream.tellg(),
                taskSize = sizeof(task),
                     pos = 0;

        // Read the tasks from our file
        task *currentTask = (task *)malloc(sizeof(task));
        while(!stream.fail() && pos + taskSize <= totalSize) {
            stream.seekg(pos);
            stream.read((char *)currentTask, sizeof(task));
            
            this->_tasks.emplace_back(*currentTask);

            pos += taskSize;
        }
        free(currentTask);

        stream.close();
    }
}

/**
 * @brief  Saves all current tasks to the specified file.
 * @param  fileName: The file to save to tasks to.
 * @retval None
 */
void TaskList::Save(std::string fileName) {
    bool isRewriting = this->_deletedTasks.size() != 0;

retry:
    std::ofstream stream(
        fileName,
        std::ios::out | std::ios::binary | (!isRewriting ? std::ios::in | std::ios::ate : std::ios::trunc)
    );

    if (!stream.good() && !isRewriting) {
        isRewriting = true;
        goto retry;
    }

    if (stream.good()) {
        size_t pos = 0;
        
        for(auto t: this->_tasks) {
            if (isRewriting && t.info.isDeleted) continue;

            if (isRewriting || t.info.isModified || t.info.isNew) {
                stream.seekp(pos);

                task fileTask = {};
                memcpy(&fileTask, &t, sizeof(task));
                memset(&fileTask.info, 0, 1);

                fileTask.info.isCompleted = t.info.isCompleted;

                stream.write((char *)&fileTask, sizeof(task));

            }

            pos += sizeof(task);
        }

        stream.flush();
        stream.close();
    }
}