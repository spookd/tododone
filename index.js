const todo = require("bindings")("todo");

class TaskError extends Error {}
class Task {

    constructor(list, data) {
        this._data = data;
        this._list = list;
    }

    _update() {
        this._list.list.edit(this._data.index, this._data.text, this._data.isCompleted);
    }

    /**
     * Delete this task from the list.
     */
    delete() {
        return this._list.delete(this);
    }

    /**
     * The index of the task.
     */
    get index() {
        return this._data.index;
    }

    /**
     * The title of the task.
     */
    get title() {
        return this._data.text;
    }

    /**
     * Sets a new title for the task.
     */
    set title(newValue) {
        if (typeof(newValue) !== "string" || newValue.trim().length === 0) {
            throw new TaskError("Title must be a string and may not be empty.");
        }

        this._data.text = newValue;
        this._update();
    }

    /**
     * Whether or not this task is completed.
     */
    get isCompleted() {
        return this._data.isCompleted;
    }

    /**
     * Sets whether or not this task is completed.
     */
    set isCompleted(newValue) {
        this._data.isCompleted = newValue === true;
        this._update();
    }

}

/**
 * A list that contains tasks that you need to do.
 */
module.exports = class TaskList {

    /**
     * A reference to the Task object.
     */
    static get Task() { return Task; }

    /**
     * Initializes a new TaskList instance.
     * @param {String} fileName The file to load and save tasks from.
     */
    constructor(fileName) {
        this._fileName = fileName;

        this._list = new todo.List();
        this._list.load(fileName);
    }

    /**
     * An instance of the native list addon.
     */
    get list() {
        return this._list;
    }

    /**
     * The filename we're working with.
     */
    get fileName() {
        return this._fileName;
    }

    /**
     * The number of tasks in total.
     */
    get count() {
        return this.list.count();
    }

    /**
     * Adds a new task to the list.
     * @param {String} title The title of the task
     */
    add(title) {
        if (typeof(title) !== "string" || title.trim().length === 0) {
            throw new TaskError("Title must be a string and may not be empty.");
        }

        return new Task(this, {
            index: this.list.add(title),
            text: title,
            isCompleted: false
        });
    }
    
    /**
     * Gets an array of tasks.
     * @param {Number} count  The number of tasks to get, -1 means all task.
     * @param {Number} offset The offset of the tasks to get. If count is -1, the offset will be ignored.
     */
    get(count = -1, offset = 0) {
        return (count === -1 ? this.list.get() : this.list.get(count, offset)).map(t => new Task(this, t));
    }
    
    /**
     * Gets an array of tasks.
     * @param {Number} count  The number of tasks to get, -1 means all task.
     * @param {Number} offset The offset of the tasks to get. If count is -1, the offset will be ignored.
     */
    get(count = -1, offset = 0) {
        return (count === -1 ? this.list.get() : this.list.get(count, offset)).map(t => new Task(this, t));
    }
    
    /**
     * Gets a specific task at the specified index.
     * @param {Number} index The index of the task to get.
     */
    getTask(index) {
        return this.get(index, 1).pop();
    }

    /**
     * Deletes a task from our list.
     * @param {TaskList.Task} taskOrIndex The task to delete, or the index of the task.
     */
    delete(taskOrIndex) {
        if (typeof(taskOrIndex) === "number") {
            this.list.delete(taskOrIndex);
        } else if (taskOrIndex instanceof Task) {
            this.list.delete(taskOrIndex.index);
        }
    }

    /**
     * Persist the tasks to disk, in the file specified.
     */
    save() {
        this.list.save(this.fileName);
    }

}