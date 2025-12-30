# Архітектура pxs3c

Мета: мобільний емулятор PS3 з фокусом на Android 10+ (Snapdragon 855+ / Adreno 640 / 4GB RAM), максимально ефективний, модульний, з апаратним прискоренням Vulkan.

## Основні модулі
- PPU: інтерфейс для JIT/інтерпретатора, майбутня інтеграція з відкритим кодом (за ліцензіями)
- SPU: інтерфейс для SPU (recompiler/interop), кешування мікрокоду
- RSX: Vulkan-бекенд (Adreno-орієнтований), компіляція шейдерів, кеш шейдерів, профайлінг
- Аудіо/I/O: стаб-інтерфейси для початку; подальша інтеграція OpenSL ES/AAudio
- Завантажувач: EBOOT/PKG/RAP обробка (roadmap)

## Продуктивність
- Vulkan only, low-overhead, descriptor indexing
- Shader pre-warm і кешування
- Frame pacing, adaptive sync, async pipeline
- CPU affinity/NUMA hints (де можливо на Android)
- Режими сумісності для 4GB RAM: агресивне кеш-обрізання, streaming

## Портативність
- Ядро C++17 + CMake
- Android NDK (LLDB/ASan дружні білди у відлагодженні)

## Ліцензії
Не копіювати зовнішній код без перевірки ліцензій. Планова база — аналіз RPCS3 (GPL), можливі альтернативи з сумісними ліцензіями. Деталі: docs/LICENSING.md.
