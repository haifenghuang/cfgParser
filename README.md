# cfgParser 
使用状态机实现的配置文件解析器

## 特性
1. 简单轻量（API函数只有三个）
2. 使用状态机实现
3. 支持全局Section和自定义Section

## API说明
此解析器提供了三个API函数供用户使用。

### int cfgParse(const char *filename)
对配置文件进行解析，参数为文件名。

### int cfgGet(const char *section, const char *key, char *out)
读取配置文件的信息。  
参数:  
    `section`: 块信息，如果为NULL，就是被认为是全局Section  
    `key`:键信息  
    `out`:键对应的值    

### void cfgFree(void)
释放解析器分配的内存空间

## 使用
配置文件请参照`test.ini`  
API使用请参照`main.c`

## 关于cfgParser3D
`cfgParser3D.c`文件是使用三维数组实现的状态机。

## Bug汇报
如果你发现程序中有任何错误，请发送邮件给我：`fenghai_hhf@163.com`。

## 许可证
MIT许可证,详细请参见LICENSE文件
