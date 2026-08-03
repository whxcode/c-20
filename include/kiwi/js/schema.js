import { ByteBuffer } from "kiwi-schema";
var schema = { ByteBuffer };

schema["decodeModel"] = function (bb) {
  var result = {};
  if (!(bb instanceof this.ByteBuffer)) {
    bb = new this.ByteBuffer(bb);
  }

  while (true) {
    switch (bb.readVarUint()) {
      case 0:
        return result;

      case 1:
        result["dataSize"] = bb.readVarUint();
        break;

      case 2:
        result["name"] = bb.readString();
        break;

      default:
        throw new Error("Attempted to parse invalid message");
    }
  }
};

schema["encodeModel"] = function (message, bb) {
  var isTopLevel = !bb;
  if (isTopLevel) bb = new this.ByteBuffer();

  var value = message["dataSize"];
  if (value != null) {
    bb.writeVarUint(1);
    bb.writeVarUint(value);
  }

  var value = message["name"];
  if (value != null) {
    bb.writeVarUint(2);
    bb.writeString(value);
  }
  bb.writeVarUint(0);

  if (isTopLevel) return bb.toUint8Array();
};

schema["decodeHttp"] = function (bb) {
  var result = {};
  if (!(bb instanceof this.ByteBuffer)) {
    bb = new this.ByteBuffer(bb);
  }

  while (true) {
    switch (bb.readVarUint()) {
      case 0:
        return result;

      case 1:
        result["host"] = bb.readString();
        break;

      case 2:
        result["url"] = bb.readString();
        break;

      case 3:
        result["contentLength"] = bb.readVarUint();
        break;

      default:
        throw new Error("Attempted to parse invalid message");
    }
  }
};

schema["encodeHttp"] = function (message, bb) {
  var isTopLevel = !bb;
  if (isTopLevel) bb = new this.ByteBuffer();

  var value = message["host"];
  if (value != null) {
    bb.writeVarUint(1);
    bb.writeString(value);
  }

  var value = message["url"];
  if (value != null) {
    bb.writeVarUint(2);
    bb.writeString(value);
  }

  var value = message["contentLength"];
  if (value != null) {
    bb.writeVarUint(3);
    bb.writeVarUint(value);
  }
  bb.writeVarUint(0);

  if (isTopLevel) return bb.toUint8Array();
};

export { schema };
