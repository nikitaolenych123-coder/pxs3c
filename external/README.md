# external/

Тут можна додати git-сабмодулі (наприклад, RPCS3) — тільки після перевірки ліцензій і згоди на GPL. Процедура:

```bash
# приклад (URL треба підтвердити ліцензійно)
git submodule add https://github.com/RPCS3/rpcs3 external/rpcs3
```

Після додавання сабмодуля потрібні адаптери CMake і узгодження інтерфейсів (див. docs/INTEGRATION.md).
