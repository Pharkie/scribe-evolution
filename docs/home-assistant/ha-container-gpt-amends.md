# Patch Summary — bring `ha-container.md` in-line with verified working setup

This single paste-ready block lists every required change, why it’s required, and the corrected snippets you can insert into `ha-container.md` (or hand to Claude for integration). It is based on the working configuration you supplied (excerpts shown below) and corrects common doc problems that break container-mounted Mosquitto setups.

## Quick reference: authoritative lines from the working setup

(These must be represented exactly in the docs)

# mosquitto (root) fragment present in working setup

MQTT configpersistence true

Socket_domain ipv4

## Authentication

Allow_anonymous false

## Include bridge configuration

Include_dir /mosquitto/config

```
(Also: host dirs used in the working setup are /opt/mosquitto/config, /opt/mosquitto/data, /opt/mosquitto/log and the container-side paths are /mosquitto/config, /mosquitto/data, /mosquitto/log.)


## 1) File paths & folder structure — exact mounts and rationale

### What to change / clarify in the doc
Replace any vague or different path examples with the exact host → container mounts used by the working setup:
- Host /opt/mosquitto/config → Container /mosquitto/config
- Host /opt/mosquitto/data   → Container /mosquitto/data
- Host /opt/mosquitto/log    → Container /mosquitto/log

Explicitly state: mount the directory (not a single file) for /opt/mosquitto/config so include_dir /mosquitto/config can load multiple fragments.

Add a short host preparation snippet (mkdir + chown + chmod) and explain why (mosquitto in-container process needs to read certs and write persistence/logs).

### Why
- Mounting a single file over /mosquitto/config hides the directory and breaks include_dir.
- Wrong container paths or single-file mounts cause mosquitto to load defaults and ignore bridge/TLS/password files.
- Permissions must allow the mosquitto process to write persistence and logs and to read keys (otherwise mosquitto may refuse keys).

### Final corrected docker-compose snippet (copy/paste)
Services:
  Mosquitto:
    Image: eclipse-mosquitto:latest
    Container_name: mosquitto
    Restart: unless-stopped
    Ports:
      - “1883:1883”
      - “8883:8883”    # optional TLS listener
      - “9001:9001”    # optional websockets
    Volumes:
      - /opt/mosquitto/config:/mosquitto/config     # config fragments, certs, passwords (directory!)
      - /opt/mosquitto/data:/mosquitto/data         # persistence DB
      - /opt/mosquitto/log:/mosquitto/log           # logs
    Environment:
-	TZ=America/Regina
```

### Host-side preparation commands (copy/paste)

`````bash
Sudo mkdir -p /opt/mosquitto/{config,data,log}
# set ownership to mosquitto uid/gid — many images run as 1883:1883
Sudo chown -R 1883:1883 /opt/mosquitto
# secure cert/key perms
Sudo chmod 644 /opt/mosquitto/config/*.crt 2>/dev/null || true
Sudo chmod 600 /opt/mosquitto/config/*.key 2>/dev/null || true
Sudo chmod 755 /opt/mosquitto /opt/mosquitto/{data,log,config}


## 2) Use include_dir /mosquitto/config (NOT include_file)

### What to change / clarify

•	Replace any include_file examples or single-file mosquitto.conf mounts with the following canonical pattern:

  * Root mosquitto.conf should include include_dir /mosquitto/config.
  * The doc must instruct that /mosquitto/config will contain multiple .conf fragments (bridge, tls, password file pointers, etc.).

•	Explain load order: files loaded alphabetically — recommend numeric prefixes (00-, 10-, 50-bridge.conf) to control ordering.

### Why

•	The working setup uses include_dir /mosquitto/config. Include_dir loads multiple config fragments; include_file or single-file mounts are incompatible with that pattern and with directory-based cert/password layouts.

### Final mosquitto root config snippet

# /mosquitto/mosquitto.conf (root)
persistence true
persistence_location /mosquitto/data/

socket_domain ipv4

# Authentication
allow_anonymous false
password_file /mosquitto/config/passwords

# load additional conf fragments (bridges, TLS, extra listeners)
include_dir /mosquitto/config


---

## 3) Bridge connection syntax, topic direction, and examples

### What to change / clarify

* Provide a complete, tested bridge fragment example file that lives inside
/opt/mosquitto/config/
(e.g.,
50-bridge.conf
).
* Document
topic
fields with the exact syntax and meaning:



  topic <pattern> <in|out|both> <qos> <local_prefix> <remote_prefix>



  and explain
in
vs
out
vs
both
.
* Recommend
cleansession true
during testing to avoid stale subscription state; document when to set it
false
permanently.
* Show remapping examples (local->remote and remote->local).

### Why

* Bridge topics are commonly misconfigured (direction inverted or wildcard forwarding causing loops).
*
cleansession
avoids surprises from lingering remote broker subscriptions.

### Final example bridge fragment (
/opt/mosquitto/config/50-bridge.conf
)
# Bridge to remote broker
connection ha-remote
address remote-broker.example.local:1883

clientid mosquitto-ha-bridge
cleansession true            # recommended while testing

# If remote broker requires credentials
remote_username myuser
remote_password mypassword

# Example: two-way remap — forward local topics under "home/" to remote and receive remote under "remote/"
topic # both 0 home/ remote/

# Or selective forwarding (local -> remote only):
# topic sensors/# out 0 sensors/ remote-sensors/


---

## 4) TLS options — where to place certs and how to reference them

### What to change / clarify

* Add explicit TLS cert-file examples that reference
/mosquitto/config/*.crt
and
/mosquitto/config/*.key
.
* Clarify file permission expectations (private keys must be restrictive) and ownership (mosquitto user).
* Show both server TLS and bridge TLS (client certificates) examples.

### Why

* If cert files are not mounted into
/mosquitto/config
or have wrong permissions, mosquitto may fail to start or refuse the cert/key. Bridge TLS settings must be present in the bridge fragment that establishes the connection.

### Final TLS snippet examples (place cert files under
/opt/mosquitto/config/
)

Server TLS (listener)
# /mosquitto/config/50-tls.conf
listener 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/server.crt
keyfile /mosquitto/config/server.key
require_certificate false


Bridge TLS (if remote broker uses TLS)
# add to /mosquitto/config/50-bridge.conf or separate fragment
address remote-broker.example.local:8883
bridge_cafile /mosquitto/config/ca.crt
bridge_certfile /mosquitto/config/client.crt
bridge_keyfile /mosquitto/config/client.key
tls_version tlsv1.2


Permission reminder (copy/paste)
sudo chown 1883:1883 /opt/mosquitto/config/*.crt /opt/mosquitto/config/*.key
sudo chmod 644 /opt/mosquitto/config/*.crt
sudo chmod 600 /opt/mosquitto/config/*.key


---

## 5) Operational steps & confusing points to add to the docs

### What to change / clarify

Add a dedicated short troubleshooting / operational section describing:

* Always mount directories (not single config files) over
/mosquitto/config
.
* After editing config fragments on the host:

  * Restart the mosquitto container:
docker restart mosquitto

  * Check container logs:
docker logs mosquitto
and host logs in
/opt/mosquitto/log
.
* If bridges do not appear, verify file load order (alphabetical) and
include_dir
usage.
* If TLS or password errors occur, verify file ownership and permissions; mosquitto may refuse to start or silently skip keys if permissions are too open or owner mismatched.

### Why

* Many failures are caused by: wrong mount type, missing restart, wrong file permissions, or wrong file load order.

### Final checklist snippet to include in docs
After changing files on the host:
1. Ensure host directories exist:
   sudo ls -ld /opt/mosquitto/{config,data,log}
2. Ensure correct mounts (see docker-compose example).
3. Ensure files are owned by mosquitto user (uid/gid 1883) and keys are chmod 600.
4. Name fragments with numeric prefixes for deterministic loading (e.g., 00-base.conf, 50-bridge.conf).
5. Restart the container: docker restart mosquitto
6. Check logs: docker logs mosquitto and /opt/mosquitto/log for runtime errors.


---

## 6) Replace or add the exact working mosquitto config fragment supplied

### What to change / clarify

Make sure the docs include the user's working mosquitto root fragment exactly, and explain where it belongs.

### Why

Having the authoritative working snippet in the docs removes ambiguity.

### Insert this exact block (copy/paste into the docs)
# Root mosquitto fragment used in working setup (place in /mosquitto/mosquitto.conf)
MQTT configpersistence true

socket_domain ipv4
## Authentication ##
allow_anonymous false

## Include bridge configuration ##
include_dir /mosquitto/config


> Note: fix the
MQTT configpersistence true
line if that is a typing/formatting issue in the repo — the standard mosquitto key is
persistence true
and
persistence_location /mosquitto/data/
. If your working container accepts that exact line, keep it; otherwise replace with:
>
>
> persistence true
> persistence_location /mosquitto/data/
>


---

## 7) Small wording changes to put in the doc (copy-ready sentences)

Replace or add near the top of the mosquitto section:

* “Mount the host directories
/opt/mosquitto/config
,
/opt/mosquitto/data
, and
/opt/mosquitto/log
into the container at
/mosquitto/config
,
/mosquitto/data
, and
/mosquitto/log
respectively. Do not mount a single file over `/mosquitto/config` — mosquitto expects a directory and the guide uses
include_dir /mosquitto/config
to load multiple config fragments.”
* “When testing bridge topic changes use
cleansession true
to avoid stale subscriptions; after validation you can set
cleansession false
if persistent subscription state is desired.”

---

## 8) Final corrected snippets (all together — paste into
ha-container.md
)

````markdown
### Mosquitto container — recommended mounts and config (copy/paste)

Docker compose (mount directories; do NOT mount a single config file)
services:
  mosquitto:
    image: eclipse-mosquitto:latest
    container_name: mosquitto
    restart: unless-stopped
    ports:
      - "1883:1883"
      - "8883:8883"
      - "9001:9001"
    volumes:
      - /opt/mosquitto/config:/mosquitto/config
      - /opt/mosquitto/data:/mosquitto/data
      - /opt/mosquitto/log:/mosquitto/log
`````

Root mosquitto.conf (inside container `/mosquitto/mosquitto.conf`)

```text
persistence true
persistence_location /mosquitto/data/

socket_domain ipv4

# Authentication
allow_anonymous false
password_file /mosquitto/config/passwords

# Load fragments
include_dir /mosquitto/config


Bridge example (
/opt/mosquitto/config/50-bridge.conf
)
connection ha-remote
address remote-broker.example.local:1883
clientid mosquitto-ha-bridge
cleansession true
remote_username myuser
remote_password mypassword

topic # both 0 home/ remote/


TLS example (
/opt/mosquitto/config/50-tls.conf
)
listener 8883
cafile /mosquitto/config/ca.crt
certfile /mosquitto/config/server.crt
keyfile /mosquitto/config/server.key
require_certificate false


Host permission prep (run on the host)
sudo mkdir -p /opt/mosquitto/{config,data,log}
sudo chown -R 1883:1883 /opt/mosquitto
sudo chmod 644 /opt/mosquitto/config/*.crt
sudo chmod 600 /opt/mosquitto/config/*.key


Troubleshooting checklist
1. Ensure directories exist & are mounted (not single file mounts).
2. Give mosquitto access (chown/chmod as above).
3. Name fragments with numeric prefixes for load order.
4. Restart container after edits: docker restart mosquitto
5. Check docker logs and /opt/mosquitto/log for errors.
```
