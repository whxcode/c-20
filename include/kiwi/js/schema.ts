// @ts-nocheck
/* eslint-disable */
import { ByteBuffer } from "kiwi-schema";

export namespace schema {
  export interface Model {
    dataSize?: number;
    name?: string;
  }

  export interface Http {
    host?: string;
    url?: string;
    contentLength?: number;
  }

  export interface Schema {
    encodeModel(message: Model): Uint8Array;
    decodeModel(buffer: Uint8Array): Model;
    encodeHttp(message: Http): Uint8Array;
    decodeHttp(buffer: Uint8Array): Http;
  }
}


const schemaRuntime = { ByteBuffer } as unknown as schema.Schema & { ByteBuffer: typeof ByteBuffer };

schemaRuntime["decodeModel"] = function (bb) {
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

schemaRuntime["encodeModel"] = function (message, bb) {
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

schemaRuntime["decodeHttp"] = function (bb) {
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

schemaRuntime["encodeHttp"] = function (message, bb) {
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


export { schemaRuntime as schema };
