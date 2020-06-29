const TaskList = require("../todo");
const taskListFileName = require("path").join(__dirname, "test.db");
const expect = require("expect.js");
const fs = require("fs");

// Ensure that we don't have an old list or fail silently <3
try {
    fs.unlinkSync(taskListFileName);
} catch(e) {}

describe("Native Addon", function() {
    beforeEach(function () {
        list = new TaskList(taskListFileName);
        list.add("One");
        list.add("Two");
        list.add("Three");
        list.save();
    });
    
    afterEach(function () {
        fs.unlinkSync(taskListFileName);
    });

    it("should throw an exception when not supplying a valid filename to load", function() {
        const fn = () => {
            const list = new TaskList();
            console.log(list);
        }

        expect(fn).to.throwError();
    });

    it("should persist tasks", function() {
        // Load tasks from file saved before
        const list = new TaskList(taskListFileName);

        expect(list.count).to.be(3);
    });

    it("should re-use old deleted tasks", function() {
        const list = new TaskList(taskListFileName);
        list.getTask(1).delete();
        list.add("Re-used");

        expect(list.getTask(1).title).to.be("Re-used");
    });

    it("should not seek beyond its bounds", function() {
        const list = new TaskList(taskListFileName);
        const result = list.get(2, 2);

        expect(result).to.be.an("array");
        expect(result.length).to.be(1);
    });

    it("should rewrite the file if we necessary", function() {
        let list;
        
        list = new TaskList(taskListFileName);
        list.getTask(1).delete();
        list.save();

        list = new TaskList(taskListFileName);

        expect(list.count).to.be(2);
    });

    it("should set a task as completed and persist it", function() {
        let list, task;

        list = new TaskList(taskListFileName);
        
        task = list.getTask(1);
        task.title = "Updated title";
        task.isCompleted = true;
        
        list.save();

        list = new TaskList(taskListFileName);
        task = list.getTask(1);

        expect(task.title).to.be("Updated title");
        expect(task.isCompleted).to.be(true);
    });
    
});
