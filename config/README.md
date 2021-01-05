absolute path!

Do ***NOT*** use `~` or environment VALUE.
In shell and c/cpp program, such `int main(int argc, char *argv[])`,
environment VALUE will be replaced in `argv[]`,
so when we pass a `~/path.file` in shell/terminal,
c/cpp will get `/home/xxx/path.file` actually.


```json
{
  "Input" : { // Must
    "AstList": "Must, path", // Default: astList.txt
    "ExtdefMapping" : "Option, path", // No Default
    "Src2Ast" : "Option, path" // No Default
  },
  "Output" : { // Must
    "Fun2Json" : "Must, path", // Default: fun2json.txt
    "Need2AnalysisPtrInfo": "Must, path", // Default: Need2AnalysisPtrInfo.txt
    "HasAnalysisPtrInfo" : "Must, path" // Default: PtrInfo.txt
  },
  "Running" : { // Option
    "IgnorePaths" : [ // Option
      "Option, path", // No Default
      "..."
    ]
  }
}
```

`class ConfigManager` will make config json fmt like as behind, no matter must/option config. Then when get config from `class ConfigManager`, it can access json without thinking about if json.contain(item), eg.
Input File:
```json
{
  "Input" : {
    "AstList": "xxx"
  }
}
```
After `class ConfigManager`:
```json
{
  "Input" : {
    "AstList": "xxx",
    "ExtdefMapping" : "",
    "Src2Ast" : "",
  }
}
```
Then, it can get any configs using `json["xxx"]["xxx"]` and judge if it is empty.