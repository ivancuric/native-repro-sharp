#include "Utils.hpp"

namespace video {

  int getInt(const Napi::Object &obj, const char *name, int emptyValue) {
    if (obj.Has(name)) {
      Napi::Value v = obj.Get(name);
      if (v.IsNumber()) {
        return v.As<Napi::Number>().Int32Value();
      }
    }
    return emptyValue;
  }

  bool getBool(const Napi::Object &obj, const char *name, bool emptyValue) {
    if (obj.Has(name)) {
      Napi::Value v = obj.Get(name);
      if (v.IsBoolean()) {
        return v.As<Napi::Boolean>().Value();
      }
    }
    return emptyValue;
  }

  std::string getString(const Napi::Object &obj, const char *name, std::string emptyValue) {
    if (obj.Has(name)) {
      Napi::Value v = obj.Get(name);
      if (v.IsString()) {
        return v.As<Napi::String>();
      }
    }
    return emptyValue;
  }
}
