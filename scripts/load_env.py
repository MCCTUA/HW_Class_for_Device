# อ่านไฟล์ .env แล้วส่งเป็น build flag (-DKEY="value") ให้ C++ ใช้งานได้
Import("env")
import os

path = os.path.join(env["PROJECT_DIR"], ".env")
if os.path.exists(path):
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        value = value.strip().strip('"').strip("'")
        env.Append(CPPDEFINES=[(key.strip(), env.StringifyMacro(value))])
else:
    print("WARNING: .env not found (copy .env.example to .env)")
