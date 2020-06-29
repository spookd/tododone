const blessed = require("blessed");

module.exports = function(list) {
    let tasks = list.get();
    const taskTitle = (task) => {
        if (task.isCompleted) {
            return `(*) ${task.title}`;
        } else {
            return `( ) ${task.title}`;
        }
    };
    const shortcuts = [
        {
            keys: ["escape", "q", "C-c"],
            fn: function() {
                list.save();
                process.exit(0);
            }
        },
        {
            keys: ["space"],
            fn: function() {
                const idx  = taskList.selected;
                const item = taskList.getItem(idx);
                const task = tasks[idx];

                task.isCompleted = !task.isCompleted;
                item.setContent(taskTitle(task));
            
                screen.render();
            }
        },
        {
            keys: ["C-a", "insert"],
            fn: function() {
                prompt.input("\n  Add a task", "", function(err, value) {
                    if (value) {
                        const task = list.add(value);
                        tasks.push(task);
                        taskList.addItem(taskTitle(task));
                    }

                    screen.render();
                });
            }
        },
        {
            keys: ["C-e"],
            fn: function() {
                const idx  = taskList.selected;
                const item = taskList.getItem(idx);
                const task = tasks[idx];

                prompt.input("\n  Edit a task", task.title, function(err, value) {
                    if (value) {
                        try {
                            task.title = value;
                            item.setContent(taskTitle(task));
                        } catch(e) {
                            message.display(e.toString(), 3);
                        }
                    } else {
                        message.display("Title must be a non-empty string.", 3);
                    }

                    screen.render();
                });
            }
        },
        {
            keys: ["delete"],
            fn: function() {
                const idx  = taskList.selected;
                const item = taskList.getItem(idx);
                const task = tasks[idx];

                task.delete();
                taskList.removeItem(idx);

                tasks.splice(idx, 1);

                screen.render();
            }
        }
    ]

    const screen = blessed.screen({
        smartCSR: true,
        autoPadding: true,
        fullUnicode: true
    });

    // Logo
    blessed.box({
        parent: screen,

        top: "0",
        left: "center",
        align: "center",
        
        width: "80%",
        height: 8,

        content: require("./logo"),
        
        border: false,
    });

    // Task box
    const taskBox = blessed.box({
        parent: screen,

        label: { text: "[ Your tasks ]", style: { fg: "orange" } },
        top: 8,
        bottom: 0,
        left: "center",
        width: "80%",
        height: "shrink",
        
        border: {
            type: "line",
            top: true,
            bottom: false,
            left: false,
            right: false
        },

        style: {
            fg: "white",
            border: {
                fg: "#ffa500",
            },
            label: {
                fg: "#ffa500"
            }
        }
    });

    // Task list
    const taskList = blessed.list({
        parent: taskBox,

        top: 0,
        width: "100%",
        height: "100%",

        keys: true,
        vi: true,
        mouse: true,

        scrollbar: {
          ch: " ",
          track: {
            bg: "cyan"
          },
          style: {
            inverse: true
          }
        },

        style: {
          item: {
            hover: {
                bg: "#888",
                bold: false
            }
          },
          selected: {
            bg: "#888",
            bold: false
          }
        }
    });

    taskList.setItems(tasks.map(task => taskTitle(task)));
    taskList.focus();

    // Prompt
    const prompt = blessed.prompt({
        parent: screen,

        top: "center",
        left: "center",
        height: "shrink",
        width: "shrink",

        hidden: true,

        keys: true,
        vi: true,
        mouse: true,
        tags: true,

        border: "line",
        
        style: {
          fg: "blue",
          bg: "black",
          bold: true,
          border: {
            fg: "blue",
          }
        }
    });

    // Message
    const message = blessed.message({
        parent: screen,

        top: "center",
        left: "center",
        height: "shrink",
        width: "shrink",

        hidden: true,

        keys: true,
        vi: true,
        mouse: true,
        tags: true,

        border: "line",
        
        style: {
          fg: "blue",
          bg: "black",
          bold: true,
          border: {
            fg: "blue",
          }
        }
    });
    

    // Hook up our keyboard shortcuts
    shortcuts.forEach(shortcut => screen.key(shortcut.keys, shortcut.fn));
    screen.render();
}