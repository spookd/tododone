{
    "targets": [
    {
        "target_name": "todo",
        "sources": [
            "src/main.cc",
            "src/task_list.cc"
        ],
        "libraries": [ "-L<(module_root_dir)/src", "-Wl,-rpath,." ],
        "include_dirs": [
            "<!(node -e \"require('nan')\")"
        ],
        "cflags": ["-w", "-fpermissive", "-fPIC"],
        "ccflags": [ "-w", "-fpermissive", "-fPIC", "-std=c++20"]
    }]
}