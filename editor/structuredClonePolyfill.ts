import { cloneDeep, findLast, findLastIndex, has, nth } from 'lodash';

/**
 * ------------------------------------------------------------------
 * Chrome 86 Polyfill 补全计划
 * 目标: 使用 Lodash 补全 Chrome 86 缺失但在 Chrome 120+ 中常用的现代 API
 * ------------------------------------------------------------------
 */

// ==========================================
// 1. 全局方法补全 (structuredClone - Chrome 98+)
// ==========================================
if (typeof globalThis.structuredClone === 'undefined') {
  globalThis.structuredClone = cloneDeep;
}

// ==========================================
// 1.1 Web Crypto 补全 (crypto.randomUUID - Chrome 92+)
// ==========================================
if (typeof globalThis.crypto !== 'undefined' && !(globalThis.crypto as Crypto).randomUUID) {
  Object.defineProperty(globalThis.crypto, 'randomUUID', {
    value: function randomUUIDPolyfill() {
      const rnds = new Uint8Array(16);
      if (globalThis.crypto?.getRandomValues) {
        globalThis.crypto.getRandomValues(rnds);
      } else {
        for (let i = 0; i < rnds.length; i += 1) {
          rnds[i] = Math.floor(Math.random() * 256);
        }
      }

      // Per RFC 4122 section 4.4
      rnds[6] = (rnds[6] & 0x0f) | 0x40; // Version 4
      rnds[8] = (rnds[8] & 0x3f) | 0x80; // Variant 10

      const byteToHex: string[] = [];
      for (let i = 0; i < 256; i += 1) {
        byteToHex[i] = (i + 0x100).toString(16).slice(1);
      }

      return `${byteToHex[rnds[0]] + byteToHex[rnds[1]] + byteToHex[rnds[2]] + byteToHex[rnds[3]]}-${
        byteToHex[rnds[4]]
      }${byteToHex[rnds[5]]}-${byteToHex[rnds[6]]}${byteToHex[rnds[7]]}-${byteToHex[rnds[8]]}${byteToHex[rnds[9]]}-${
        byteToHex[rnds[10]]
      }${byteToHex[rnds[11]]}${byteToHex[rnds[12]]}${byteToHex[rnds[13]]}${byteToHex[rnds[14]]}${byteToHex[rnds[15]]}`;
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// ==========================================
// 2. 静态方法补全 (Object.hasOwn - Chrome 93+)
// ==========================================
if (!(Object as any).hasOwn) {
  Object.defineProperty(Object, 'hasOwn', {
    value: function hasOwnPolyfill(object: any, property: PropertyKey) {
      return has(object, property);
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// ==========================================
// 3. 数组/字符串原型链补全 (Array/String)
// ==========================================

// --- Array.prototype.with ---
// 用法: arr.with(2, 'b') -> 返回把索引2改成'b'的新数组
if (!(Array.prototype as any).with) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(Array.prototype, 'with', {
    value: function withPolyfill(index: number, value: any) {
      const copy = [...this]; // 浅拷贝
      // 处理负索引
      const i = index < 0 ? index + copy.length : index;
      if (i >= 0 && i < copy.length) {
        copy[i] = value;
      } else {
        throw new RangeError(`Invalid index : ${index}`);
      }
      return copy;
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// --- Array.prototype.toSorted (Chrome 110+) ---
// 返回排序后的新数组，不修改原数组
if (!(Array.prototype as any).toSorted) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(Array.prototype, 'toSorted', {
    value: function toSortedPolyfill(compareFn?: (a: any, b: any) => number) {
      const copy = [...this];
      return copy.sort(compareFn as any);
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// --- Array.prototype.toReversed (Chrome 110+) ---
// 返回反转后的新数组，不修改原数组
if (!(Array.prototype as any).toReversed) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(Array.prototype, 'toReversed', {
    value: function toReversedPolyfill() {
      const copy = [...this];
      return copy.reverse();
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// --- Array.prototype.toSpliced (Chrome 110+) ---
// 返回 splice 后的新数组，不修改原数组
if (!(Array.prototype as any).toSpliced) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(Array.prototype, 'toSpliced', {
    value: function toSplicedPolyfill(start: number, deleteCount?: number, ...items: any[]) {
      const copy = [...this];
      const len = copy.length;
      const actualStart = start < 0 ? Math.max(len + start, 0) : Math.min(start, len);
      const actualDeleteCount =
        deleteCount === undefined ? len - actualStart : Math.max(0, Math.min(deleteCount, len - actualStart));
      copy.splice(actualStart, actualDeleteCount, ...items);
      return copy;
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// --- Array.prototype.at (Chrome 92+) ---
if (!Array.prototype.at) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(Array.prototype, 'at', {
    value: function atPolyfill(n: number) {
      return nth(this, n);
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// --- String.prototype.at (Chrome 92+) ---
// 很多人忘了 String 也有 at，支持 'abc'.at(-1)
if (!String.prototype.at) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(String.prototype, 'at', {
    value: function atPolyfill(n: number) {
      // Lodash 的 nth 也支持字符串处理
      return nth(this, n);
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// --- Array.prototype.findLast (Chrome 97+) ---
// 从后往前查找元素
if (!(Array.prototype as any).findLast) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(Array.prototype, 'findLast', {
    value: function findLastPolyfill(predicate: any) {
      return findLast(this, predicate);
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}

// --- Array.prototype.findLastIndex (Chrome 97+) ---
// 从后往前查找元素的索引
if (!(Array.prototype as any).findLastIndex) {
  // eslint-disable-next-line no-extend-native
  Object.defineProperty(Array.prototype, 'findLastIndex', {
    value: function findLastIndexPolyfill(predicate: any) {
      return findLastIndex(this, predicate);
    },
    writable: true,
    enumerable: false,
    configurable: true,
  });
}
