#!/bin/bash
set -e
cd "$(dirname "$0")"

mkdir -p include js tmp

# 1. npm 依赖
[ ! -d node_modules ] && npm install

# 2. kiwi 运行时
[ ! -f include/kiwi.h ] && wget -q https://raw.githubusercontent.com/evanw/kiwi/master/cpp/kiwi.h -O include/kiwi.h

# 3. 合并所有 kiwi 文件
files=$(find schema -name "*.kiwi")
cat $files >tmp/schema.kiwi
echo 'package schema;' | cat - tmp/schema.kiwi > tmp/schema.kiwi.new && mv tmp/schema.kiwi.new tmp/schema.kiwi

# 4. 生成 C++ 头文件
npx kiwic --schema tmp/schema.kiwi --cpp include/schema.h
{ echo '#pragma once'; echo ''; cat include/schema.h; } > include/schema.h.new && mv include/schema.h.new include/schema.h
echo "✅ include/schema.h"

# 5. 生成 JS 解码器（并转为 ESM 去掉 require）
npx kiwic --schema tmp/schema.kiwi --js js/schema.js
{
  echo 'import { ByteBuffer } from "kiwi-schema";'
  echo 'var schema = { ByteBuffer };'
  tail -n +3 js/schema.js
} > js/schema.js.new && mv js/schema.js.new js/schema.js
echo '' >>js/schema.js
echo 'export { schema };' >>js/schema.js
echo "✅ js/schema.js (ESM)"

# 6. 生成 TS 类型
npx kiwic --schema tmp/schema.kiwi --ts js/schema.d.ts
echo '' >>js/schema.d.ts
echo 'export const schema: schema.Schema;' >>js/schema.d.ts
echo "✅ js/schema.d.ts"

# 7. 生成 TS 运行时
{
  echo '// @ts-nocheck'
  echo '/* eslint-disable */'
  echo 'import { ByteBuffer } from "kiwi-schema";'
  echo ''
  sed '/^export const schema:/d' js/schema.d.ts
  echo ''
  sed \
    -e '1d' \
    -e 's/^var schema = { ByteBuffer };/const schemaRuntime = { ByteBuffer } as unknown as schema.Schema \& { ByteBuffer: typeof ByteBuffer };/g' \
    -e 's/schema\[/schemaRuntime[/g' \
    -e '/^export { schema };$/d' \
    js/schema.js
  echo ''
  echo 'export { schemaRuntime as schema };'
} >js/schema.ts
echo "✅ js/schema.ts"

# 8. 清理
rm -rf tmp
