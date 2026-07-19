# RetailSystems для клиента 12.0.7

Изолированный слой для Chromie Time, Challenge Mode / Mythic+ и BattlePay. Пакеты и структуры клиентских данных проверены по `Wow.exe` 12.0.7.68453; DB2 и GameTables читаются из результата свежего `mapextractor`, без изменений базовой hotfix-схемы TrinityCore.

## Установка

1. Соберите `mapextractor` и извлеките как минимум DB2 (`-e 4`) и GameTables (`-e 8`) из того же клиента. Для полного стандартного набора можно использовать `-e 10`.
2. Скопируйте `retail_systems.conf.dist` в каталог рядом с `worldserver.conf` как `worldserver.conf.d/retail_systems.conf`. Значение `RetailSystems.ClientDataDir` должно указывать на корень, содержащий `dbc/<locale>` и `gt`.
3. Оставьте штатные `Updates.EnableDatabases` и `Updates.AutoSetup` включёнными. DB-updater TrinityCore автоматически найдёт миграции в:
   - `sql/custom/auth/retail_systems`;
   - `sql/custom/characters/retail_systems`;
   - `sql/custom/world/retail_systems`.
4. Пересоберите сервер:

   ```powershell
   cmake -S . -B build
   cmake --build build --config RelWithDebInfo --target worldserver -j 6
   ```

Для текущей машины готовый путь данных — `C:/Games/World of Warcraft/_retail_`; там уже присутствуют оба нужных каталога и таблицы масштабирования Challenge Mode.

На Windows перед запуском `worldserver.exe` в `PATH` должны быть каталоги с `libmysql.dll` и OpenSSL. Для текущей установки это `C:/Program Files/MySQL/MySQL Server 8.0/lib` и `C:/Program Files/OpenSSL-Win64/bin`; сами библиотеки также можно положить рядом с executable.

## Что реализовано

- Chromie Time: открытие штатного UI через gossip, выбор/выход из временной линии, свежие CMSG/SMSG, update fields, content-tuning flags и сохранение состояния персонажа.
- Mythic+: запуск и сброс ключа, отсчёт, таймер, смерти и штрафы, сохранение/повышение/понижение ключа, клиентские DB2/GameTables, сезон и ротация аффиксов, масштабирование существ, завершение сценария/боссов, история карт, score и guild/realm leaderboards.
- BattlePay: свежий каталог, группы, bundles/deliverables, история и баланс аккаунта, двухэтапное подтверждение цены, транзакционное списание, защита цели покупки и выдача предметов, заклинаний, маунтов и игрушек.

BattlePay по умолчанию выключен. Отдельная world-миграция создаёт тестовый товар
`1001`, выдающий 20 единиц льняного материала. Баланс realm назначает оператор:

```sql
INSERT INTO auth.retail_battlepay_balance (account_id, balance)
VALUES (1, 1000000)
ON DUPLICATE KEY UPDATE balance = VALUES(balance);
```

Для клиента 12.x можно воспроизвести storefront и граф `CatalogShop` из своего
официального PKT 3.1-дампа. Сгенерируйте из него capture-каталог и укажите файл
в `RetailSystems.BattlePay.CaptureFile`:

```powershell
python src/server/game/Custom/RetailSystems/BattlePay/tools/extract_capture_catalog.py `
  dump_12.0.7.68453.pkt battlepay_12.0.7.68453.tcbp
```

Capture-каталог отвечает только за клиентское наполнение storefront. Выдача
купленного товара по-прежнему разрешена исключительно для записей custom DB,
прошедших серверную валидацию.

## Быстрая проверка Chromie Time

World-миграция добавляет пункт `Выбрать временную линию` официальной Chromie
`entry 167032` с gossip-меню `25426`. В актуальной TDB её spawn в Штормграде
имеет GUID `8000063`; к нему можно перейти GM-командой:

```text
.go creature 8000063
```

Если spawn отсутствует в другой версии TDB, создайте Chromie рядом с персонажем:

```text
.npc add 167032
```

После выбора временной линии состояние появляется в таблице characters
`retail_character_chromie_time` и восстанавливается при следующем входе.

## Обновление из TrinityCore

Работайте в отдельной feature-ветке и регулярно переносите её поверх апстрима:

```powershell
git fetch upstream
git rebase upstream/master
cmake -S . -B build
cmake --build build --config RelWithDebInfo --target worldserver -j 6
```

Основной объём находится в новых каталогах `game/Custom/RetailSystems`, `scripts/Custom/RetailSystems` и `sql/custom/*/retail_systems`. В файлах TrinityCore оставлены только небольшие точки подключения: регистрация opcode/handler, загрузка кастомных DB2 и scripts, Chromie spell/update fields, определение активного Mythic+ map и две GameTable-записи extractor. Это удерживает конфликты при rebase локальными и обозримыми.

## Границы текущей реализации

Специфические игровые скрипты сезонных аффиксов 148/158/160/162, Mythic+ loot/chest и Great Vault не входят в общий протокольный слой и требуют отдельной реализации под контент конкретного сезона. BattlePay не выполняет внешние платежи: баланс полностью серверный и управляется оператором realm.
