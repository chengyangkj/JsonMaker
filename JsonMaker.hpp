#pragma once
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace JsonMaker {

/******************************************************
 * Register class or struct members
 * eg:
 * struct Test
 * {
 *      string A;
 *      string B;
 *      JSON_MAKER_REGISTER(A, B)
 * };
 ******************************************************/
#define JSON_MAKER_REGISTER(...)                                           \
  std::map<std::string, std::string> __DefaultValues;                      \
  bool RegisterJsonToObject(JsonMaker::JsonHandlerPrivate &handle,         \
                            rapidjson::Value &jsonValue,                   \
                            std::vector<std::string> &names) {             \
    std::vector<std::string> standardNames =                               \
        handle.GetMembersNames(#__VA_ARGS__);                              \
    if (names.size() <= standardNames.size()) {                            \
      for (int i = names.size(); i < (int)standardNames.size(); i++)       \
        names.push_back(standardNames[i]);                                 \
    }                                                                      \
    return handle.SetMembers(names, 0, jsonValue, __DefaultValues,         \
                             __VA_ARGS__);                                 \
  }                                                                        \
  bool RegisterObjectToJson(JsonMaker::JsonHandlerPrivate &handle,         \
                            rapidjson::Value &jsonValue,                   \
                            rapidjson::Document::AllocatorType &allocator, \
                            std::vector<std::string> &names) {             \
    std::vector<std::string> standardNames =                               \
        handle.GetMembersNames(#__VA_ARGS__);                              \
    if (names.size() <= standardNames.size()) {                            \
      for (int i = names.size(); i < (int)standardNames.size(); i++)       \
        names.push_back(standardNames[i]);                                 \
    }                                                                      \
    return handle.GetMembers(names, 0, jsonValue, allocator, __VA_ARGS__); \
  }

#define JSON_MAKE_SERIALIZATION()                                \
  std::string GetStrFromObject() {                               \
    std::string strResult = "";                                  \
    JsonMaker::Maker::ObjectToJson((*this), strResult);          \
    return strResult;                                            \
  }                                                              \
  bool SetStrToObject(std::string strData) {                     \
    bool ret = JsonMaker::Maker::JsonToObject((*this), strData); \
    if (strData.length() <= 0 && !ret) {                         \
      return false;                                              \
    }                                                            \
    return true;                                                 \
  }

/******************************************************
 * 处理数组类型，没有预先分配内存指针，在反序列化中需要提前分配大小
 * eg:
 * struct Test
 * {
 *      int array1[6];  固定大小
 *      double* array2; 没有预先分配内存，也没有用于标记大小的值
 *      int* array3; 没有预先分配内存，len用于标记大小的值
 *       int len = 0; 标记array3的大小
 *      JSON_MAKER_ARRAY(array1, 6, array2, 0, array3, len) //
 *0表示初始没有给大小
 * };
 ******************************************************/
#define JSON_MAKER_ARRAY(...)                                               \
  std::map<std::string, int> __ArrayItem;                                   \
  std::map<uintptr_t, int> __ArrayAdress;                                   \
  std::map<uintptr_t, std::pair<std::string, int>> __ArrayInfo;             \
  std::vector<std::pair<std::string, int>> __Arrays;                        \
  bool InitArrayInfo(JsonMaker::JsonHandlerPrivate &handle) {               \
    std::vector<std::string> standardNames =                                \
        handle.GetMembersNames(#__VA_ARGS__);                               \
    if (standardNames.size() % 2 != 0) {                                    \
      return false;                                                         \
    }                                                                       \
                                                                            \
    std::vector<int> nums;                                                  \
    std::vector<uintptr_t> ptrs;                                            \
    bool ret = handle.GetArrayMembersSize(nums, ptrs, __VA_ARGS__);         \
    if (!ret) {                                                             \
      return false;                                                         \
    }                                                                       \
                                                                            \
    if (nums.size() * 2 != standardNames.size()) {                          \
      return false;                                                         \
    }                                                                       \
    for (int i, j = 0; i < standardNames.size() && j < nums.size();         \
         i = i + 2, j++) {                                                  \
      std::pair<std::string, int> pairTemp;                                 \
      pairTemp.first = standardNames[i];                                    \
      pairTemp.second = nums[j];                                            \
      __ArrayItem[standardNames[i]] = nums[j];                              \
      auto it = __ArrayAdress.find(ptrs[j]);                                \
      if (it != __ArrayAdress.end()) {                                      \
        __ArrayItem[standardNames[i]] = __ArrayAdress[ptrs[j]];             \
        pairTemp.second = __ArrayAdress[ptrs[j]];                           \
      }                                                                     \
      __ArrayInfo[ptrs[j]] = pairTemp;                                      \
      __Arrays.push_back(pairTemp);                                         \
    }                                                                       \
    return handle.SaveArrayInfo(__ArrayItem);                               \
  }                                                                         \
  bool RegisterArrayToJson(JsonMaker::JsonHandlerPrivate &handle,           \
                           rapidjson::Value &jsonValue,                     \
                           rapidjson::Document::AllocatorType &allocator) { \
    if (!InitArrayInfo(handle)) {                                           \
      return false;                                                         \
    }                                                                       \
    return handle.GetArrayInfo(__ArrayInfo, jsonValue, allocator,           \
                               __VA_ARGS__);                                \
  }                                                                         \
  bool SetArrayParamsSize(void *ptr, int count) {                           \
    uintptr_t uPtr = reinterpret_cast<uintptr_t>(ptr);                      \
    __ArrayAdress[uPtr] = count;                                            \
    return true;                                                            \
  }                                                                         \
                                                                            \
  int GetArrayParamsSize(std::string paramName, std::string strData) {      \
    int size = -1;                                                          \
    rapidjson::Document root;                                               \
    root.Parse(strData.c_str());                                            \
    if (root.IsNull()) {                                                    \
      return size;                                                          \
    }                                                                       \
    rapidjson::Value &value = root;                                         \
    if (value.HasMember(paramName.c_str())) {                               \
      size = value[paramName.c_str()].GetArray().Size();                    \
    }                                                                       \
    return size;                                                            \
  }                                                                         \
  bool RegisterJsonToArray(JsonMaker::JsonHandlerPrivate &handle,           \
                           rapidjson::Value &jsonValue) {                   \
    if (!InitArrayInfo(handle)) {                                           \
      return false;                                                         \
    }                                                                       \
    return handle.SetArrayInfo(__Arrays, 0, jsonValue, __VA_ARGS__);        \
  }

/******************************************************
 * Rename members
 * eg:
 * struct Test
 * {
 *      string A;
 *      string B;
 *      JSON_MAKER_REGISTER(A, B)
 *      JSON_MAKER_RENAME("a", "b")
 * };
 ******************************************************/
#define JSON_MAKER_RENAME(...)                    \
  std::vector<std::string> RegisterRenameMembers( \
      JsonMaker::JsonHandlerPrivate &handle) {    \
    return handle.GetMembersNames(#__VA_ARGS__);  \
  }

/******************************************************
 * Register base-class
 * eg:
 * struct Base
 * {
 *      string name;
 *      JSON_MAKER_REGISTER(name)
 * };
 * struct Test : Base
 * {
 *      string A;
 *      string B;
 *      JSON_MAKER_REGISTER(A, B)
 *      JSON_MAKER_REGISTER_BASE((Base*)this)
 * };
 ******************************************************/
#define JSON_MAKER_REGISTER_BASE(...)                                     \
  bool RegisterBaseJsonToObject(JsonMaker::JsonHandlerPrivate &handle,    \
                                rapidjson::Value &jsonValue) {            \
    return handle.SetBase(jsonValue, __VA_ARGS__);                        \
  }                                                                       \
  bool RegisterBaseObjectToJson(                                          \
      JsonMaker::JsonHandlerPrivate &handle, rapidjson::Value &jsonValue, \
      rapidjson::Document::AllocatorType &allocator) {                    \
    return handle.GetBase(jsonValue, allocator, __VA_ARGS__);             \
  }

/******************************************************
 * Set default value
 * eg:
 * struct Base
 * {
 *      string name;
 *      int age;
 *      JSON_MAKER_REGISTER(name, age)
 *      JSON_MAKER_REGISTER_DEFAULT(age=18)
 * };
 ******************************************************/
#define JSON_MAKER_REGISTER_DEFAULT(...)                              \
  void RegisterDefaultValues(JsonMaker::JsonHandlerPrivate &handle) { \
    __DefaultValues = handle.GetMembersValueMap(#__VA_ARGS__);        \
  }

class JsonHandlerPrivate {
 public:
  /******************************************************
   *
   * enable_if
   *
   ******************************************************/
  template <bool, class TYPE = void>
  struct enable_if {};

  template <class TYPE>
  struct enable_if<true, TYPE> {
    typedef TYPE type;
  };

  /******************************************************
   *
   * 判断是否为指针
   *
   ******************************************************/
  template <typename T>
  bool IsPtr(T *v) {
    return true;
  }

  bool IsPtr(...) { return false; }

  std::set<std::string> m_arrayMark;  //记录原始数组的标识
  bool SaveArrayInfo(std::map<std::string, int> &mapInfo) {
    for (auto it = mapInfo.begin(); it != mapInfo.end(); ++it) {
      m_arrayMark.insert(it->first);
    }

    return true;
  }

 public:
  /******************************************************
   *
   * Check Interface
   *      If class or struct add
   *JSON_MAKER_REGISTER\JSON_MAKER_RENAME\JSON_MAKER_REGISTER_BASE, it will
   *go to the correct conver function.
   *
   ******************************************************/
  template <typename T>
  struct HasConverFunction {
    template <typename TT>
    static char func(decltype(&TT::RegisterJsonToObject));

    template <typename TT>
    static int func(...);

    const static bool has = (sizeof(func<T>(NULL)) == sizeof(char));
  };

  template <typename T>
  struct HasRenameFunction {
    template <typename TT>
    static char func(decltype(&TT::RegisterRenameMembers));
    template <typename TT>
    static int func(...);
    const static bool has = (sizeof(func<T>(NULL)) == sizeof(char));
  };

  template <typename T>
  struct HasBaseConverFunction {
    template <typename TT>
    static char func(decltype(&TT::RegisterBaseJsonToObject));
    template <typename TT>
    static int func(...);
    const static bool has = (sizeof(func<T>(NULL)) == sizeof(char));
  };

  template <typename T>
  struct HasDefaultValueFunction {
    template <typename TT>
    static char func(decltype(&TT::RegisterDefaultValues));
    template <typename TT>
    static int func(...);
    const static bool has = (sizeof(func<T>(NULL)) == sizeof(char));
  };

  template <typename T>
  struct HasArrayToJsonRegisterFunction {
    template <typename TT>
    static char func(decltype(&TT::RegisterArrayToJson));
    template <typename TT>
    static int func(...);
    const static bool has = (sizeof(func<T>(NULL)) == sizeof(char));
  };

  template <typename T>
  struct HasJsonToArrayRegisterFunction {
    template <typename TT>
    static char func(decltype(&TT::RegisterArrayToJson));
    template <typename TT>
    static int func(...);
    const static bool has = (sizeof(func<T>(NULL)) == sizeof(char));
  };

 public:
  /******************************************************
   *
   * Interface of JsonToObject\ObjectToJson
   *
   ******************************************************/
  template <typename T,
            typename enable_if<HasConverFunction<T>::has, int>::type = 0>
  bool JsonToObject(T &obj, rapidjson::Value &jsonValue) {
    if (!BaseConverJsonToObject(obj, jsonValue)) return false;

    LoadDefaultValuesMap(obj);
    LoadJsonToArrayInfo(obj, jsonValue);
    std::vector<std::string> names = LoadRenameArray(obj);
    return obj.RegisterJsonToObject(*this, jsonValue, names);
  }

  template <typename T,
            typename enable_if<!HasConverFunction<T>::has, int>::type = 0>
  bool JsonToObject(T &obj, rapidjson::Value &jsonValue) {
    bool ret = HandleSpecialObjectToJson(obj, jsonValue);
    if (ret) {
      return ret;
    }

    m_message = "unsupported this type.";
    return false;
  }

  template <typename T,
            typename enable_if<std::is_enum<T>::value, int>::type = 0>
  bool HandleSpecialObjectToJson(T &obj, rapidjson::Value &jsonValue) {
    int64_t ivalue;
    if (!JsonToObject(ivalue, jsonValue)) return false;

    obj = static_cast<T>(ivalue);
    return true;
  }

  template <typename T,
            typename enable_if<!std::is_enum<T>::value, int>::type = 0>
  bool HandleSpecialObjectToJson(T &obj, rapidjson::Value &jsonValue) {
    if (!BaseConverJsonToObject(obj, jsonValue)) return false;
    LoadDefaultValuesMap(obj);
    LoadJsonToArrayInfo(obj, jsonValue);
    return true;
  }

  //解决static_cast语法检查问题
  template <typename T,
            typename enable_if<!HasConverFunction<T>::has, int>::type = 0>
  bool JsonToObject(T *obj, rapidjson::Value &jsonValue) {
    return false;
  }

  template <typename T,
            typename enable_if<HasConverFunction<T>::has, int>::type = 0>
  bool ObjectToJson(T &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    if (jsonValue.IsNull()) jsonValue.SetObject();
    if (!BaseConverObjectToJson(obj, jsonValue, allocator)) return false;
    std::vector<std::string> names = LoadRenameArray(obj);
    LoadArrayInfo(obj, jsonValue, allocator);
    return obj.RegisterObjectToJson(*this, jsonValue, allocator, names);
  }

  template <typename T,
            typename enable_if<!HasConverFunction<T>::has, int>::type = 0>
  bool ObjectToJson(T &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    if (jsonValue.IsNull()) jsonValue.SetObject();
    if (!BaseConverObjectToJson(obj, jsonValue, allocator)) return false;
    bool ret = LoadArrayInfo(obj, jsonValue, allocator);
    ret = HandleSpecialObjectToJson(obj, jsonValue, allocator);
    // if (std::is_enum<T>::value) {
    //   int ivalue = static_cast<int>(obj);
    //   return ObjectToJson(ivalue, jsonValue, allocator);
    // }

    if (!ret) {
      m_message = "unsupported this type.";
    }

    return ret;
  }

  template <typename T,
            typename enable_if<std::is_enum<T>::value, int>::type = 0>
  bool HandleSpecialObjectToJson(
      T &obj, rapidjson::Value &jsonValue,
      rapidjson::Document::AllocatorType &allocator) {
    int64_t ivalue = static_cast<int64_t>(obj);
    return ObjectToJson(ivalue, jsonValue, allocator);
  }

  template <typename T,
            typename enable_if<!std::is_enum<T>::value, int>::type = 0>
  bool HandleSpecialObjectToJson(
      T &obj, rapidjson::Value &jsonValue,
      rapidjson::Document::AllocatorType &allocator) {
    return true;
  }

  //解决static_cast语法检查问题
  template <typename T,
            typename enable_if<!HasConverFunction<T>::has, int>::type = 0>
  bool ObjectToJson(T *obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    return false;
  }

  /******************************************************
   *
   * Interface of LoadRenameArray
   *
   ******************************************************/
  template <typename T,
            typename enable_if<HasRenameFunction<T>::has, int>::type = 0>
  std::vector<std::string> LoadRenameArray(T &obj) {
    return obj.RegisterRenameMembers(*this);
  }

  template <typename T,
            typename enable_if<!HasRenameFunction<T>::has, int>::type = 0>
  std::vector<std::string> LoadRenameArray(T &obj) {
    return std::vector<std::string>();
  }

  /******************************************************
   *
   * Interface of BaseConverJsonToObject\BaseConverObjectToJson
   *
   ******************************************************/
  template <typename T,
            typename enable_if<HasBaseConverFunction<T>::has, int>::type = 0>
  bool BaseConverJsonToObject(T &obj, rapidjson::Value &jsonValue) {
    return obj.RegisterBaseJsonToObject(*this, jsonValue);
  }

  template <typename T,
            typename enable_if<!HasBaseConverFunction<T>::has, int>::type = 0>
  bool BaseConverJsonToObject(T &obj, rapidjson::Value &jsonValue) {
    return true;
  }

  template <typename T,
            typename enable_if<HasBaseConverFunction<T>::has, int>::type = 0>
  bool BaseConverObjectToJson(T &obj, rapidjson::Value &jsonValue,
                              rapidjson::Document::AllocatorType &allocator) {
    return obj.RegisterBaseObjectToJson(*this, jsonValue, allocator);
  }

  template <typename T,
            typename enable_if<!HasBaseConverFunction<T>::has, int>::type = 0>
  bool BaseConverObjectToJson(T &obj, rapidjson::Value &jsonValue,
                              rapidjson::Document::AllocatorType &allocator) {
    return true;
  }

  /******************************************************
   *
   * Interface of Default value
   *
   ******************************************************/
  template <typename T,
            typename enable_if<HasDefaultValueFunction<T>::has, int>::type = 0>
  void LoadDefaultValuesMap(T &obj) {
    obj.RegisterDefaultValues(*this);
  }

  template <typename T,
            typename enable_if<!HasDefaultValueFunction<T>::has, int>::type = 0>
  void LoadDefaultValuesMap(T &obj) {
    (void)obj;
  }

  /******************************************************
   *
   * 将原始数组信息转化为json
   *
   ******************************************************/
  template <
      typename T,
      typename enable_if<HasArrayToJsonRegisterFunction<T>::has, int>::type = 0>
  bool LoadArrayInfo(T &obj, rapidjson::Value &jsonValue,
                     rapidjson::Document::AllocatorType &allocator) {
    return obj.RegisterArrayToJson(*this, jsonValue, allocator);
  }

  template <typename T,
            typename enable_if<!HasArrayToJsonRegisterFunction<T>::has,
                               int>::type = 0>
  bool LoadArrayInfo(T &obj, rapidjson::Value &jsonValue,
                     rapidjson::Document::AllocatorType &allocator) {
    return false;
  }

  template <
      typename T,
      typename enable_if<HasJsonToArrayRegisterFunction<T>::has, int>::type = 0>
  bool LoadJsonToArrayInfo(T &obj, rapidjson::Value &jsonValue) {
    return obj.RegisterJsonToArray(*this, jsonValue);
  }

  template <typename T,
            typename enable_if<!HasJsonToArrayRegisterFunction<T>::has,
                               int>::type = 0>
  bool LoadJsonToArrayInfo(T &obj, rapidjson::Value &jsonValue) {
    return false;
  }

 public:
  /******************************************************
   *
   * Tool function
   *
   ******************************************************/
  static std::vector<std::string> StringSplit(const std::string &str,
                                              char sep = ',') {
    std::vector<std::string> array;
    std::string::size_type pos1, pos2;
    pos1 = 0;
    pos2 = str.find(sep);
    while (std::string::npos != pos2) {
      array.push_back(str.substr(pos1, pos2 - pos1));
      pos1 = pos2 + 1;
      pos2 = str.find(sep, pos1);
    }
    if (pos1 != str.length()) array.push_back(str.substr(pos1));

    return array;
  }

  static std::string StringTrim(std::string key) {
    std::string newStr = key;
    if (!newStr.empty()) {
      newStr.erase(0, newStr.find_first_not_of(" "));
      newStr.erase(newStr.find_last_not_of(" ") + 1);
    }
    if (!newStr.empty()) {
      newStr.erase(0, newStr.find_first_not_of("\""));
      newStr.erase(newStr.find_last_not_of("\"") + 1);
    }
    return newStr;
  }

  static void StringTrim(std::vector<std::string> &array) {
    for (int i = 0; i < (int)array.size(); i++) {
      array[i] = StringTrim(array[i]);
    }
  }

  /**
   * Get json value type
   */
  static std::string GetJsonValueTypeName(rapidjson::Value &jsonValue) {
    switch (jsonValue.GetType()) {
      case rapidjson::Type::kArrayType:
        return "array";
      case rapidjson::Type::kFalseType:
      case rapidjson::Type::kTrueType:
        return "bool";
      case rapidjson::Type::kObjectType:
        return "object";
      case rapidjson::Type::kStringType:
        return "string";
      case rapidjson::Type::kNumberType:
        return "number";
      default:
        return "string";
    }
  }

  static std::string GetStringFromJsonValue(rapidjson::Value &jsonValue) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    jsonValue.Accept(writer);
    std::string ret = std::string(buffer.GetString());
    return ret;
  }

  static std::string FindStringFromMap(
      std::string name, std::map<std::string, std::string> &stringMap) {
    std::map<std::string, std::string>::iterator iter = stringMap.find(name);
    if (iter == stringMap.end()) return "";
    return iter->second;
  }

 public:
  /******************************************************
   *
   * Set class/struct members value
   *
   ******************************************************/

  template <typename TYPE, typename SIZE, typename... TYPES>
  bool SetArrayInfo(std::vector<std::pair<std::string, int>> &arrays, int index,
                    rapidjson::Value &jsonValue, TYPE &&arg, SIZE &&count,
                    TYPES &&...args) {
    if (!SetArrayInfo(arrays, index, jsonValue, arg, count)) {
      return false;
    }

    return SetArrayInfo(arrays, ++index, jsonValue, args...);
  }

  template <typename TYPE, typename SIZE>
  bool SetArrayInfo(std::vector<std::pair<std::string, int>> &arrays, int index,
                    rapidjson::Value &jsonValue, TYPE &&arg, SIZE &&count) {
    std::string strKey = arrays[index].first;
    if (!jsonValue.HasMember(strKey.c_str())) {
      return false;
    }
    if (!jsonValue[strKey.c_str()].IsArray()) {
      return false;
    }
    int size = jsonValue[strKey.c_str()].GetArray().Size();
    if (arrays[index].second != size) {
      return false;
    }

    for (int i = 0; i < size; i++) {
      if (!JsonToObject(arg[i], jsonValue[strKey.c_str()][i])) return false;
    }

    return true;
  }

  /******************************************************
   *
   * 将原始数组转到json
   *
   ******************************************************/
  template <typename TYPE, typename SIZE, typename... TYPES>
  bool GetArrayInfo(std::map<uintptr_t, std::pair<std::string, int>> &mapInfo,
                    rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator, TYPE &&arg,
                    SIZE &&count, TYPES &&...args) {
    if (!GetArrayInfo(mapInfo, jsonValue, allocator, arg, count)) {
      return false;
    }

    return GetArrayInfo(mapInfo, jsonValue, allocator, args...);
  }

  template <typename TYPE, typename SIZE>
  bool GetArrayInfo(std::map<uintptr_t, std::pair<std::string, int>> &mapInfo,
                    rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator, TYPE &arg,
                    SIZE &&count) {
    rapidjson::Value array(rapidjson::Type::kArrayType);
    int size = count;
    std::string strKey = "";
    auto it = mapInfo.find(reinterpret_cast<uintptr_t>(arg));
    if (it != mapInfo.end()) {
      size = it->second.second;
      strKey = it->second.first;
    }

    for (int i = 0; i < size; i++) {
      rapidjson::Value item;
      if (!ObjectToJson(arg[i], item, allocator)) return false;

      array.PushBack(item, allocator);
    }

    if (count < 0) {
      return true;
    }

    if (jsonValue.HasMember(strKey.c_str())) {
      jsonValue.RemoveMember(strKey.c_str());
    }

    rapidjson::Value key;
    printf("strKey == %s\n", strKey.c_str());
    key.SetString(strKey.c_str(), strKey.length(), allocator);
    jsonValue.AddMember(key, array, allocator);

    return true;
  }

  template <typename TYPE, typename SIZE, typename... TYPES>
  bool GetArrayMembersSize(std::vector<int> &nums, std::vector<uintptr_t> &ptrs,
                           TYPE &&arg, SIZE &&count, TYPES &&...args) {
    if (!GetArrayMembersSize(nums, ptrs, arg, count)) {
      return false;
    }
    if (!GetArrayMembersSize(nums, ptrs, args...)) {
      return false;
    }
    return true;
  }

  template <typename TYPE, typename SIZE>
  bool GetArrayMembersSize(std::vector<int> &nums, std::vector<uintptr_t> &ptrs,
                           TYPE &&arg, SIZE &&count) {
    bool ret = IsPtr(arg);
    nums.push_back(count);
    ptrs.push_back(reinterpret_cast<uintptr_t>(arg));
    return true;
  }

  std::vector<std::string> GetMembersNames(const std::string membersStr) {
    std::vector<std::string> array = StringSplit(membersStr);
    StringTrim(array);
    return array;
  }

  std::map<std::string, std::string> GetMembersValueMap(
      const std::string valueStr) {
    std::vector<std::string> array = StringSplit(valueStr);
    std::map<std::string, std::string> ret;
    for (int i = 0; i < array.size(); i++) {
      std::vector<std::string> keyValue = StringSplit(array[i], '=');
      if (keyValue.size() != 2) continue;

      std::string key = StringTrim(keyValue[0]);
      std::string value = StringTrim(keyValue[1]);
      if (ret.find(key) != ret.end()) continue;
      ret.insert(std::pair<std::string, std::string>(key, value));
    }
    return ret;
  }

  template <typename TYPE, typename... TYPES>
  bool SetMembers(const std::vector<std::string> &names, int index,
                  rapidjson::Value &jsonValue,
                  std::map<std::string, std::string> defaultValues, TYPE &arg,
                  TYPES &...args) {
    if (!SetMembers(names, index, jsonValue, defaultValues, arg)) return false;

    return SetMembers(names, ++index, jsonValue, defaultValues, args...);
  }

  template <typename TYPE>
  bool SetMembers(const std::vector<std::string> &names, int index,
                  rapidjson::Value &jsonValue,
                  std::map<std::string, std::string> defaultValues, TYPE &arg) {
    if (m_arrayMark.count(names[index]) > 0) {
      //为原始数组数据，这里不做处理
      return true;
    }
    if (jsonValue.IsNull()) return true;
    const char *key = names[index].c_str();
    if (!jsonValue.IsObject()) return false;
    if (!jsonValue.HasMember(key)) {
      std::string defaultV = FindStringFromMap(names[index], defaultValues);
      if (!defaultV.empty()) StringToObject(arg, defaultV);
      return true;
    }

    if (!JsonToObject(arg, jsonValue[key])) {
      m_message = "[" + names[index] + "] " + m_message;
      return false;
    }
    return true;
  }

  /******************************************************
   *
   * Get class/struct members value
   *
   ******************************************************/
  template <typename TYPE, typename... TYPES>
  bool GetMembers(const std::vector<std::string> &names, int index,
                  rapidjson::Value &jsonValue,
                  rapidjson::Document::AllocatorType &allocator, TYPE &arg,
                  TYPES &...args) {
    if (!GetMembers(names, index, jsonValue, allocator, arg)) return false;
    return GetMembers(names, ++index, jsonValue, allocator, args...);
  }

  template <typename TYPE>
  bool GetMembers(const std::vector<std::string> &names, int index,
                  rapidjson::Value &jsonValue,
                  rapidjson::Document::AllocatorType &allocator, TYPE &arg) {
    if (m_arrayMark.count(names[index]) > 0) {
      //为原始数组数据，这里不做处理
      return true;
    }
    rapidjson::Value item;
    bool check = ObjectToJson(arg, item, allocator);
    if (!check) {
      m_message = "[" + names[index] + "] " + m_message;
      return false;
    }

    if (jsonValue.HasMember(names[index].c_str())) {
      jsonValue.RemoveMember(names[index].c_str());
    }

    rapidjson::Value key;
    key.SetString(names[index].c_str(), names[index].length(), allocator);
    jsonValue.AddMember(key, item, allocator);
    return true;
  }

 public:
  /******************************************************
   *
   * Set base class value
   *
   ******************************************************/
  template <typename TYPE, typename... TYPES>
  bool SetBase(rapidjson::Value &jsonValue, TYPE *arg, TYPES *...args) {
    if (!SetBase(jsonValue, arg)) return false;
    return SetBase(jsonValue, args...);
  }

  template <typename TYPE>
  bool SetBase(rapidjson::Value &jsonValue, TYPE *arg) {
    return JsonToObject(*arg, jsonValue);
  }

  /******************************************************
   *
   * Get base class value
   *
   ******************************************************/
  template <typename TYPE, typename... TYPES>
  bool GetBase(rapidjson::Value &jsonValue,
               rapidjson::Document::AllocatorType &allocator, TYPE *arg,
               TYPES *...args) {
    if (!GetBase(jsonValue, allocator, arg)) return false;
    return GetBase(jsonValue, allocator, args...);
  }

  template <typename TYPE>
  bool GetBase(rapidjson::Value &jsonValue,
               rapidjson::Document::AllocatorType &allocator, TYPE *arg) {
    return ObjectToJson(*arg, jsonValue, allocator);
  }

 public:
  /******************************************************
   * Conver base-type : string to base-type
   * Contain: int\uint、int64_t\uint64_t、bool、float
   *          double、string
   *
   ******************************************************/
  template <typename TYPE>
  void StringToObject(TYPE &obj, std::string &value) {
    return;
  }

  void StringToObject(std::string &obj, std::string &value) { obj = value; }

  void StringToObject(int &obj, std::string &value) {
    obj = atoi(value.c_str());
  }

  void StringToObject(unsigned int &obj, std::string &value) {
    char *end;
    obj = strtoul(value.c_str(), &end, 10);
  }

  void StringToObject(int64_t &obj, std::string &value) {
    char *end;
    obj = strtoll(value.c_str(), &end, 10);
  }

  void StringToObject(uint64_t &obj, std::string &value) {
    char *end;
    obj = strtoull(value.c_str(), &end, 10);
  }

  void StringToObject(bool &obj, std::string &value) {
    obj = (value == "true");
  }

  void StringToObject(float &obj, std::string &value) {
    obj = atof(value.c_str());
  }

  void StringToObject(double &obj, std::string &value) {
    obj = atof(value.c_str());
  }

 public:
  /******************************************************
   * Conver base-type : Json string to base-type
   * Contain: int\uint、int64_t\uint64_t、bool、float
   *          double、string、vector、list、map<string,XX>
   *
   ******************************************************/
  bool JsonToObject(int &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsInt()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is int.";
      return false;
    }
    obj = jsonValue.GetInt();
    return true;
  }

  bool JsonToObject(unsigned int &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsUint()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is unsigned int.";
      return false;
    }
    obj = jsonValue.GetUint();
    return true;
  }

  bool JsonToObject(int64_t &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsInt64()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is int64_t.";
      return false;
    }
    obj = jsonValue.GetInt64();
    return true;
  }
  bool JsonToObject(uint8_t &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsInt()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is int64_t.";
      return false;
    }
    obj = jsonValue.GetInt();
    return true;
  }
  bool JsonToObject(uint16_t &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsInt()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is int64_t.";
      return false;
    }
    obj = jsonValue.GetInt();
    return true;
  }
  bool JsonToObject(int8_t &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsInt()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is int64_t.";
      return false;
    }
    obj = jsonValue.GetInt();
    return true;
  }
  bool JsonToObject(int16_t &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsInt()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is int64_t.";
      return false;
    }
    obj = jsonValue.GetInt();
    return true;
  }

  bool JsonToObject(uint64_t &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsUint64()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is uint64_t.";
      return false;
    }
    obj = jsonValue.GetUint64();
    return true;
  }

  bool JsonToObject(bool &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsBool()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is bool.";
      return false;
    }
    obj = jsonValue.GetBool();
    return true;
  }

  bool JsonToObject(float &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsNumber()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is float.";
      return false;
    }
    obj = jsonValue.GetFloat();
    return true;
  }

  bool JsonToObject(double &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsNumber()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is double.";
      return false;
    }
    obj = jsonValue.GetDouble();
    return true;
  }

  bool JsonToObject(std::string &obj, rapidjson::Value &jsonValue) {
    obj = "";
    if (jsonValue.IsNull()) return true;
    // object or number conver to string
    else if (jsonValue.IsObject() || jsonValue.IsNumber())
      obj = GetStringFromJsonValue(jsonValue);
    else if (!jsonValue.IsString()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is string.";
      return false;
    } else
      obj = jsonValue.GetString();

    return true;
  }
  template <typename TYPE>
  bool JsonToObject(std::vector<TYPE> &obj, rapidjson::Value &jsonValue) {
    obj.clear();
    if (!jsonValue.IsArray()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is std::vector<TYPE>.";
      return false;
    }

    auto array = jsonValue.GetArray();
    for (int i = 0; i < array.Size(); i++) {
      TYPE item;
      if (!JsonToObject(item, array[i])) return false;
      obj.push_back(item);
    }
    return true;
  }
  template <typename TYPE>
  bool JsonToObject(std::forward_list<TYPE> &obj, rapidjson::Value &jsonValue) {
    obj.clear();
    if (!jsonValue.IsArray()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is std::vector<TYPE>.";
      return false;
    }

    auto array = jsonValue.GetArray();
    for (int i = 0; i < array.Size(); i++) {
      TYPE item;
      if (!JsonToObject(item, array[i])) return false;
      obj.push_front(item);
    }
    obj.reverse();
    return true;
  }
  template <typename TYPE>
  bool JsonToObject(std::deque<TYPE> &obj, rapidjson::Value &jsonValue) {
    obj.clear();
    if (!jsonValue.IsArray()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is std::vector<TYPE>.";
      return false;
    }

    auto array = jsonValue.GetArray();
    for (int i = 0; i < array.Size(); i++) {
      TYPE item;
      if (!JsonToObject(item, array[i])) return false;
      obj.push_back(item);
    }
    return true;
  }

  template <typename TYPE>
  bool JsonToObject(std::list<TYPE> &obj, rapidjson::Value &jsonValue) {
    obj.clear();
    if (!jsonValue.IsArray()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is std::list<TYPE>.";
      return false;
    }

    auto array = jsonValue.GetArray();
    for (int i = 0; i < array.Size(); i++) {
      TYPE item;
      if (!JsonToObject(item, array[i])) return false;
      obj.push_back(item);
    }
    return true;
  }

  template <typename TYPE>
  bool JsonToObject(std::map<std::string, TYPE> &obj,
                    rapidjson::Value &jsonValue) {
    obj.clear();
    if (!jsonValue.IsObject()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is std::map<std::string, TYPE>.";
      return false;
    }

    for (auto iter = jsonValue.MemberBegin(); iter != jsonValue.MemberEnd();
         ++iter) {
      auto key = (iter->name).GetString();
      auto &value = jsonValue[key];

      TYPE item;
      if (!JsonToObject(item, value)) return false;

      obj.insert(std::pair<std::string, TYPE>(key, item));
    }
    return true;
  }

  template <typename TYPE>
  bool JsonToObject(std::shared_ptr<TYPE> &obj, rapidjson::Value &jsonValue) {
    if (!jsonValue.IsObject()) {
      m_message = "json-value is " + GetJsonValueTypeName(jsonValue) +
                  " but object is std::map<std::string, TYPE>.";
      return false;
    }

    TYPE temp;
    if (!JsonToObject(temp, jsonValue)) {
      return false;
    }

    TYPE *t = new TYPE(std::move(temp));
    obj.reset(t);

    return true;
  }

 public:
  /******************************************************
   * Conver base-type : base-type to json string
   * Contain: int\uint、int64_t\uint64_t、bool、float
   *          double、string、vector、list、map<string,XX>
   *
   ******************************************************/
  bool ObjectToJson(int &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetInt(obj);
    return true;
  }

  bool ObjectToJson(unsigned int &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetUint(obj);
    return true;
  }
  bool ObjectToJson(int8_t &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetInt(obj);
    return true;
  }
  bool ObjectToJson(int16_t &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetInt(obj);
    return true;
  }
  bool ObjectToJson(uint8_t &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetInt(obj);
    return true;
  }
  bool ObjectToJson(uint16_t &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetInt(obj);
    return true;
  }
  bool ObjectToJson(int64_t &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetInt64(obj);
    return true;
  }

  bool ObjectToJson(uint64_t &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetUint64(obj);
    return true;
  }

  bool ObjectToJson(bool &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetBool(obj);
    return true;
  }

  bool ObjectToJson(float &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetFloat(obj);
    return true;
  }

  bool ObjectToJson(double &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetDouble(obj);
    return true;
  }

  bool ObjectToJson(std::string &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetString(obj.c_str(), obj.length(), allocator);
    return true;
  }

  template <typename TYPE>
  bool ObjectToJson(std::vector<TYPE> &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    rapidjson::Value array(rapidjson::Type::kArrayType);
    for (int i = 0; i < obj.size(); i++) {
      rapidjson::Value item;
      if (!ObjectToJson(obj[i], item, allocator)) return false;

      array.PushBack(item, allocator);
    }

    jsonValue = array;
    return true;
  }

  template <typename TYPE>
  bool ObjectToJson(std::list<TYPE> &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    rapidjson::Value array(rapidjson::Type::kArrayType);
    for (auto i = obj.begin(); i != obj.end(); i++) {
      rapidjson::Value item;
      if (!ObjectToJson(*i, item, allocator)) return false;

      array.PushBack(item, allocator);
    }

    jsonValue = array;
    return true;
  }
  template <typename TYPE>
  bool ObjectToJson(std::forward_list<TYPE> &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    rapidjson::Value array(rapidjson::Type::kArrayType);
    for (auto i = obj.begin(); i != obj.end(); i++) {
      rapidjson::Value item;
      if (!ObjectToJson(*i, item, allocator)) return false;

      array.PushBack(item, allocator);
    }

    jsonValue = array;
    return true;
  }
  template <typename TYPE>
  bool ObjectToJson(std::deque<TYPE> &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    rapidjson::Value array(rapidjson::Type::kArrayType);
    for (auto i = obj.begin(); i != obj.end(); i++) {
      rapidjson::Value item;
      if (!ObjectToJson(*i, item, allocator)) return false;

      array.PushBack(item, allocator);
    }

    jsonValue = array;
    return true;
  }

  template <typename TYPE>
  bool ObjectToJson(std::map<std::string, TYPE> &obj,
                    rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetObject();
    for (auto iter = obj.begin(); iter != obj.end(); ++iter) {
      auto key = iter->first;
      TYPE value = iter->second;

      rapidjson::Value jsonitem;
      if (!ObjectToJson(value, jsonitem, allocator)) return false;

      rapidjson::Value jsonkey;
      jsonkey.SetString(key.c_str(), key.length(), allocator);

      jsonValue.AddMember(jsonkey, jsonitem, allocator);
    }
    return true;
  }

  template <typename TYPE>
  bool ObjectToJson(std::shared_ptr<TYPE> &obj, rapidjson::Value &jsonValue,
                    rapidjson::Document::AllocatorType &allocator) {
    jsonValue.SetObject();
    if (obj.use_count() <= 0) {
      return true;
    }

    if (!ObjectToJson(*obj.get(), jsonValue, allocator)) return false;

    return true;
  }

 public:
  std::string m_message;
};

class Maker {
 public:
  /**
   * @brief conver json string to class | struct
   * @param obj : class or struct
   * @param jsonStr : json string
   * @param keys : the path of the object
   * @param message : printf err message when conver failed
   */
  template <typename T>
  static bool JsonToObject(T &obj, const std::string &jsonStr,
                           const std::vector<std::string> keys = {},
                           std::string *message = NULL) {
    // Parse json string
    rapidjson::Document root;
    root.Parse(jsonStr.c_str());
    if (root.IsNull()) {
      if (message) *message = "Json string can't parse.";
      return false;
    }

    // Go to the key-path
    std::string path;
    rapidjson::Value &value = root;
    for (int i = 0; i < (int)keys.size(); i++) {
      const char *find = keys[i].c_str();
      if (!path.empty()) path += "->";
      path += keys[i];

      if (!value.IsObject() || !value.HasMember(find)) {
        if (message) *message = "Can't parse the path [" + path + "].";
        return false;
      }
      value = value[find];
    }

    // Conver
    JsonHandlerPrivate handle;
    if (!handle.JsonToObject(obj, value)) {
      if (message) *message = handle.m_message;
      return false;
    }
    return true;
  }

  /**
   * @brief conver json string to class | struct
   * @param jsonStr : json string
   * @param defaultT : default value
   * @param keys : the path of the object
   * @param message : printf err message when conver failed
   */
  template <typename T>
  static T Get(const std::string &jsonStr, T defaultT,
               const std::vector<std::string> keys = {},
               std::string *message = NULL) {
    T obj;
    if (JsonToObject(obj, jsonStr, keys, message)) return obj;

    return defaultT;
  }

  /**
   * @brief conver class | struct to json string
   * @param errMessage : printf err message when conver failed
   * @param obj : class or struct
   * @param jsonStr : json string
   */
  template <typename T>
  static bool ObjectToJson(T &obj, std::string &jsonStr,
                           std::string *message = NULL) {
    rapidjson::Document root;
    root.SetObject();
    rapidjson::Document::AllocatorType &allocator = root.GetAllocator();

    // Conver
    JsonHandlerPrivate handle;
    if (!handle.ObjectToJson(obj, root, allocator)) {
      if (message) *message = handle.m_message;
      return false;
    }

    jsonStr = handle.GetStringFromJsonValue(root);
    return true;
  }
};

}  // namespace JsonMaker
