# 问题记录

1. `std::enable_shared_from_this` 一定要用 `public` 继承，否则使用`shared_from_this`会抛出 `std::bad_weak_ptr`。
2. `#include`的作用等于`copy`、`paste`。