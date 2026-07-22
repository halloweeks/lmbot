# Lords Mobile Bot Configuration

> **Note:** Configuration support is currently under development.  
> Not all options are available or fully supported yet. More configuration options will be added as development continues.

This document describes the available configuration options for **Lords Mobile Bot**.

## Creating Configuration

A default configuration file can be generated using:

```bash
./client --create-config
```

This creates:

```text
config.cfg
```

in the current working directory.

After creation, edit `config.cfg` to configure the bot.

The bot loads the configuration file from the current directory when starting:

```bash
./client
```

## Configuration Format

The configuration file uses a simple key-value format:

```cfg
key = value
```

Lines beginning with `#` are comments.

Example:

```cfg
server.addr = 192.243.44.63
server.port = 5999
command.prefix = $
```

---

## Gateway Server

Controls the gateway server connection.

```cfg
server.addr = 192.243.44.63
server.port = 5999
```

| Option | Description |
|---|---|
| `server.addr` | Gateway server address |
| `server.port` | Gateway server port |

---

## Client Version

Defines the game client version and language.

```cfg
client.version_major = 2
client.version_minor = 197
client.version_patch = 308
client.language_code = 1
```

| Option | Description |
|---|---|
| `client.version_major` | Major client version |
| `client.version_minor` | Minor client version |
| `client.version_patch` | Patch version |
| `client.language_code` | Client language code |

---

## Data Path

Directory used to store bot data such as logs, databases, and cache.

```cfg
data.path = ./data/
```

---

## Administrator

Defines the privileged player.

This player can execute administrator commands and bypass normal restrictions.

```cfg
admin.name = halloweeks
```

---

## Account

Account information used for authentication.

```cfg
account.igg_id = YOUR_IGG_ID
account.device_uuid = YOUR_DEVICE_UUID
account.access_key = YOUR_ACCESS_KEY
```

> **Warning:** Do not share your account credentials publicly.

---

## Command System

Controls bot command handling.

```cfg
command.prefix = $
```

### Command Channels

Available channels:

```
WORLD
GUILD
MAIL
```

Example:

```cfg
command.input = GUILD
command.output = MAIL
```

| Option | Description |
|---|---|
| `command.prefix` | Prefix used for bot commands |
| `command.input` | Channel where commands are received |
| `command.output` | Channel where responses are sent |

---

## Banking System

The banking system handles resource transfer commands.

### Enable Banking

Master switch for the banking system.

```cfg
bank.enabled = false
```

When disabled, all banking commands are ignored.

### Allowed Resources

Controls which resources can be delivered.

```cfg
bank.send_food = false
bank.send_rock = false
bank.send_wood = false
bank.send_ore = false
bank.send_gold = false
```

### Resource Reserve

Reserved resources that the bot will not send.

```cfg
bank.reserve_food = 20M
bank.reserve_rock = 50M
bank.reserve_wood = 50M
bank.reserve_ore = 30M
bank.reserve_gold = 0
```

### Delivery Distance

Maximum map distance for resource delivery.

```cfg
bank.max_delivery_distance = 100
```

### Use Resource Items

Allows using resource items from the bag if required.

```cfg
bank.use_bag_rss = false

bank.use_bag_food = false
bank.use_bag_rock = false
bank.use_bag_wood = false
bank.use_bag_ore = false
bank.use_bag_gold = false
```

---

## Protection System

Controls automatic defensive features.

### Enable Protection

```cfg
protection.enabled = false
```

### Shield Settings

```cfg
protection.shield_always_on = false
protection.shield_on_incoming_attack = false
protection.shield_on_incoming_scout = false
```

### Shield Priority

The bot uses the first available shield in this order.

```cfg
protection.shield_priority = SHIELD_4H, SHIELD_8H, SHIELD_12H, SHIELD_1D
```

Available shields:

```
SHIELD_4H
SHIELD_8H
SHIELD_12H
SHIELD_1D
SHIELD_3D
SHIELD_7D
SHIELD_14D
```

### March Recall

Automatically recalls marches when threats are detected.

```cfg
protection.recall_on_incoming_attack = false
protection.recall_on_incoming_scout = false
protection.recall_on_incoming_conflict = false
```

---

## Shelter System

Currently unavailable.

Future options:

```cfg
# protection.shelter_always = false
# protection.shelter_leader = true
# protection.shelter_troops = false
# protection.shelter_on_incoming_attack = true
# protection.shelter_on_incoming_scout = true
```

---

## Cargo Ship

Automatically completes Cargo Ship trades.

### Enable Trading

```cfg
cargo_ship.auto_trade = false
```

### Allowed Resources

```cfg
cargo_ship.spend_food = false
cargo_ship.spend_rock = false
cargo_ship.spend_wood = false
cargo_ship.spend_ore = false
cargo_ship.spend_gold = false
```

### Use Resource Items

```cfg
cargo_ship.use_bag_rss = false
```

### Resource Reserve

The bot keeps these amounts and only spends excess resources.

```cfg
cargo_ship.reserve_food = 10M
cargo_ship.reserve_rock = 10M
cargo_ship.reserve_wood = 10M
cargo_ship.reserve_ore = 10M
cargo_ship.reserve_gold = 10M
```

---

## Future Configuration

The following features are planned or under development:

- Additional protection options
- Shelter automation
- More resource management options
- Additional bot modules
- More runtime configuration controls