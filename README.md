# 结构体或类序列化与反序列化为Json
## 一，基础使用
导入头文件
```cpp
#include "inc/JsonMaker/JsonMaker.hpp"
```
### 1，注册结构体或类需要序列化的成员变量：

需要注意成员变量的权限需要为public(结构体默认为public,类需要声明为public)
使用JSON_MAKER_REGISTER宏
```cpp
class Student
{
public:
    string Name;
    int Age;

    JSON_MAKER_REGISTER(Name, Age)
};
```
### 2，Json转为Object
```cpp
    Student person;
    JsonMaker::Maker::JsonToObject(person, R"({"Name":"XiaoMing", "Age":15})");
```
### 3，object转为Json
```cpp
    string jsonStr;
    JsonMaker::Maker::ObjectToJson(person, jsonStr);
```
### 二,结构体或类基类多继承转换
支持继承转换，需要注册基类，并且在基类中注册成员
使用JSON_MAKER_REGISTER_BASE宏
```cpp
class Student : public Person, Classes {
 public:
  string Name;
  int Age;

  JSON_MAKER_REGISTER(Name, Age)
  JSON_MAKER_REGISTER_BASE((Person*)this, (Classes*)this)
};
```
如果想要递归使用基类，则每一级的基类都需要注册:
```cpp

using namespace std;
class Animal {
 private:
  /* data */
 public:
  string type = "人";
  JSON_MAKER_REGISTER(type)
};
class Person : public Animal {
 private:
  /* data */
 public:
  int id;
  string gender = "男";
  JSON_MAKER_REGISTER(id, gender)
  JSON_MAKER_REGISTER_BASE((Animal*)this)
};
class Classes {
 private:
  /* data */
 public:
  string class_id = "1024";
  JSON_MAKER_REGISTER(class_id)
};
class Student : public Person, Classes {
 public:
  string Name;
  int Age;

  JSON_MAKER_REGISTER(Name, Age)
  JSON_MAKER_REGISTER_BASE((Person*)this, (Classes*)this)
};

int main(int argc, char* argv[]) {
  Student stu;
  string result;
  JsonMaker::Maker::ObjectToJson(stu, result);
  std::cout << result << std::endl;
//   {"type":"人","id":0,"gender":"男","class_id":"1024","Name":"","Age":65535}
  return 1;
}

```
## 三,变量重命名

默认以变量名作为json键名，如果需要重命名，通过JSON_MAKER_RENAME宏:

注意需要依次重命名所有的变量

```cpp
class Student : public Person {
 public:
  string Name;
  int Age;
  int member;
  JSON_MAKER_REGISTER(Name, Age, member)
  JSON_MAKER_RENAME("names", "ages", "members")
  JSON_MAKER_REGISTER_BASE((Person*)this)
};

```
## 四，支持的基本数据类型

注意:如果结构体定义中的类型不在下面的中，则会导致转换失败,转换结果为空:

支持json原生类型:

```cpp
int
uint
int64_t
uint64_t
bool
float
double
string
unsigned int (uint32_t)

```

兼容的类型:

由于json原生不支持这些类型，如果结构体成员定义为这些类型则会将json中的数据由int转为对应类型:

```cpp

int8_t
int16_t
uint8_t
uint16_t

```

支持的键值类容器:
```cpp
std::map<std::string,XXX>
```

## 支持的STL容器数据类型
支持的序列化容器：

```cpp
std::list
std::string
std::vector
std::forward_list
std::deque
```

支持的键值类容器:
```cpp
std::map<std::string,XXX>
```

## 五，数据类型扩展

如果自己想要的数据类型不在上面定义中,可以自行扩展,在JsonMake.hpp中进行扩展:

* json转结构体的扩展:

重载JsonMake.hpp中的JsonToObject函数,比如我想增加int32类型的支持，只需要在JsonMake.hpp新增如下重载即可:
其中需要自行写json对象到类型的转换，我这里将Json int类型转换为对应的类型：

```cpp

  bool JsonToObject(int32_t &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsInt()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is int64_t.";
      return false;
    }
    obj = jsonValue.GetInt();
    return true;
  }

```


* 结构体转json的扩展:

重载JsonMake.hpp中的JsonToObject函数,比如我想增加int32类型的支持，只需要在JsonMake.hpp新增如下重载即可:

其中需要自行写json对象到类型的转换，我这里将Json int类型转换为对应的类型：

```cpp

  bool ObjectToJson(int32_t &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetInt(obj);
    return true;
  }

```