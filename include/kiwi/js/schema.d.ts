export namespace schema {
  export interface Http {
    host?: string;
    url?: string;
    contentLength?: number;
  }

  export interface Model {
    dataSize?: number;
    name?: string;
  }

  export interface Schema {
    encodeHttp(message: Http): Uint8Array;
    decodeHttp(buffer: Uint8Array): Http;
    encodeModel(message: Model): Uint8Array;
    decodeModel(buffer: Uint8Array): Model;
  }
}

export const schema: schema.Schema;
