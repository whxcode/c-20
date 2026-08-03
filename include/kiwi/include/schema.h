#pragma once

#include "kiwi.h"

namespace schema {

#ifndef INCLUDE_SCHEMA_H
#define INCLUDE_SCHEMA_H

class BinarySchema {
public:
  bool parse(kiwi::ByteBuffer &bb);
  const kiwi::BinarySchema &underlyingSchema() const { return _schema; }
  bool skipModelField(kiwi::ByteBuffer &bb, uint32_t id) const;
  bool skipHttpField(kiwi::ByteBuffer &bb, uint32_t id) const;

private:
  kiwi::BinarySchema _schema;
  uint32_t _indexModel = 0;
  uint32_t _indexHttp = 0;
};

class Model;
class Http;

class Model {
public:
  Model() { (void)_flags; }

  uint32_t *dataSize();
  const uint32_t *dataSize() const;
  void set_dataSize(const uint32_t &value);

  kiwi::String *name();
  const kiwi::String *name() const;
  void set_name(const kiwi::String &value);

  bool encode(kiwi::ByteBuffer &bb);
  bool decode(kiwi::ByteBuffer &bb, kiwi::MemoryPool &pool, const BinarySchema *schema = nullptr);

private:
  uint32_t _flags[1] = {};
  kiwi::String _data_name = {};
  uint32_t _data_dataSize = {};
};

class Http {
public:
  Http() { (void)_flags; }

  kiwi::String *host();
  const kiwi::String *host() const;
  void set_host(const kiwi::String &value);

  kiwi::String *url();
  const kiwi::String *url() const;
  void set_url(const kiwi::String &value);

  uint32_t *contentLength();
  const uint32_t *contentLength() const;
  void set_contentLength(const uint32_t &value);

  bool encode(kiwi::ByteBuffer &bb);
  bool decode(kiwi::ByteBuffer &bb, kiwi::MemoryPool &pool, const BinarySchema *schema = nullptr);

private:
  uint32_t _flags[1] = {};
  kiwi::String _data_host = {};
  kiwi::String _data_url = {};
  uint32_t _data_contentLength = {};
};

#endif
#ifdef IMPLEMENT_SCHEMA_H

bool BinarySchema::parse(kiwi::ByteBuffer &bb) {
  if (!_schema.parse(bb)) return false;
  _schema.findDefinition("Model", _indexModel);
  _schema.findDefinition("Http", _indexHttp);
  return true;
}

bool BinarySchema::skipModelField(kiwi::ByteBuffer &bb, uint32_t id) const {
  return _schema.skipField(bb, _indexModel, id);
}

bool BinarySchema::skipHttpField(kiwi::ByteBuffer &bb, uint32_t id) const {
  return _schema.skipField(bb, _indexHttp, id);
}

uint32_t *Model::dataSize() {
  return _flags[0] & 1 ? &_data_dataSize : nullptr;
}

const uint32_t *Model::dataSize() const {
  return _flags[0] & 1 ? &_data_dataSize : nullptr;
}

void Model::set_dataSize(const uint32_t &value) {
  _flags[0] |= 1; _data_dataSize = value;
}

kiwi::String *Model::name() {
  return _flags[0] & 2 ? &_data_name : nullptr;
}

const kiwi::String *Model::name() const {
  return _flags[0] & 2 ? &_data_name : nullptr;
}

void Model::set_name(const kiwi::String &value) {
  _flags[0] |= 2; _data_name = value;
}

bool Model::encode(kiwi::ByteBuffer &_bb) {
  if (dataSize() != nullptr) {
    _bb.writeVarUint(1);
    _bb.writeVarUint(_data_dataSize);
  }
  if (name() != nullptr) {
    _bb.writeVarUint(2);
    _bb.writeString(_data_name.c_str());
  }
  _bb.writeVarUint(0);
  return true;
}

bool Model::decode(kiwi::ByteBuffer &_bb, kiwi::MemoryPool &_pool, const BinarySchema *_schema) {
  while (true) {
    uint32_t _type;
    if (!_bb.readVarUint(_type)) return false;
    switch (_type) {
      case 0:
        return true;
      case 1: {
        if (!_bb.readVarUint(_data_dataSize)) return false;
        set_dataSize(_data_dataSize);
        break;
      }
      case 2: {
        if (!_bb.readString(_data_name, _pool)) return false;
        set_name(_data_name);
        break;
      }
      default: {
        if (!_schema || !_schema->skipModelField(_bb, _type)) return false;
        break;
      }
    }
  }
}

kiwi::String *Http::host() {
  return _flags[0] & 1 ? &_data_host : nullptr;
}

const kiwi::String *Http::host() const {
  return _flags[0] & 1 ? &_data_host : nullptr;
}

void Http::set_host(const kiwi::String &value) {
  _flags[0] |= 1; _data_host = value;
}

kiwi::String *Http::url() {
  return _flags[0] & 2 ? &_data_url : nullptr;
}

const kiwi::String *Http::url() const {
  return _flags[0] & 2 ? &_data_url : nullptr;
}

void Http::set_url(const kiwi::String &value) {
  _flags[0] |= 2; _data_url = value;
}

uint32_t *Http::contentLength() {
  return _flags[0] & 4 ? &_data_contentLength : nullptr;
}

const uint32_t *Http::contentLength() const {
  return _flags[0] & 4 ? &_data_contentLength : nullptr;
}

void Http::set_contentLength(const uint32_t &value) {
  _flags[0] |= 4; _data_contentLength = value;
}

bool Http::encode(kiwi::ByteBuffer &_bb) {
  if (host() != nullptr) {
    _bb.writeVarUint(1);
    _bb.writeString(_data_host.c_str());
  }
  if (url() != nullptr) {
    _bb.writeVarUint(2);
    _bb.writeString(_data_url.c_str());
  }
  if (contentLength() != nullptr) {
    _bb.writeVarUint(3);
    _bb.writeVarUint(_data_contentLength);
  }
  _bb.writeVarUint(0);
  return true;
}

bool Http::decode(kiwi::ByteBuffer &_bb, kiwi::MemoryPool &_pool, const BinarySchema *_schema) {
  while (true) {
    uint32_t _type;
    if (!_bb.readVarUint(_type)) return false;
    switch (_type) {
      case 0:
        return true;
      case 1: {
        if (!_bb.readString(_data_host, _pool)) return false;
        set_host(_data_host);
        break;
      }
      case 2: {
        if (!_bb.readString(_data_url, _pool)) return false;
        set_url(_data_url);
        break;
      }
      case 3: {
        if (!_bb.readVarUint(_data_contentLength)) return false;
        set_contentLength(_data_contentLength);
        break;
      }
      default: {
        if (!_schema || !_schema->skipHttpField(_bb, _type)) return false;
        break;
      }
    }
  }
}

#endif

}
