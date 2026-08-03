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

export const schema: schema.Schema;
