![version](https://img.shields.io/badge/version-20%2B-E23089)
![platform](https://img.shields.io/static/v1?label=platform&message=mac-intel%20|%20mac-arm%20|%20win-64&color=blue)
[![license](https://img.shields.io/github/license/miyako/fastcdc)](LICENSE)
![downloads](https://img.shields.io/github/downloads/miyako/fastcdc/total)

# fastcdc
Tool to split data into content determined chunks

> [!WARNING]
> CDC on text may yield semantically incorrect chunks.

Using [xiacode/FastCDC-c](https://github.com/wxiacode/FastCDC-c)

CLI is a [`text-splitter`](https://github.com/miyako/text-splitter) alternative.

> [!TIP]
> although intended for text chunking, the algorithm can be used on arbitrary data too. pass the `-r` switch to process raw data

> [!NOTE]
> by default, the `ORIGIN_CDC` function is used. other functions are available via switches  
> 
> `-R`: `ROLLING_2Bytes`  
> `-n`: `NORMALIZED_CDC`  
> `-N`: `NORMALIZED_2Bytes`  

## usage

```
Usage:  fastcdc -r -i in -o out -

split text into content determined chunks

 -i path  : document to parse
 -o path  : text output (default=stdout)
 -        : use stdin for input
 -r       : raw data (default=no)
 -f size  : minimum size (default=2048)
 -t size  : maximum size (default=32768)
```
