# onm-cli [WIP]

"Okda Network CLI" (onmcli) is a command-line interface designed 
for configuring the Sysrepo configuration database. 
It operates by generating commands based on the YANG modules
that have been installed in the Sysrepo system.

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)
- [License](#license)

## Installation
onmcli uses a customized version of [libcli](https://github.com/dparrish/libcli)

```bash
# install custom libcli
$ make install_lib
# build
$ make
# run
$ ./onm_cli
```
## usage

[+] usage example `onmcli`:

```commandline
$ ./onmcli 
[INF] Connection 394 created.
[INF] Session 381 (user "ali", CID 394) created.


onmcli version: 1.0.0
by Okda networks (c) 2023
router> en
router# sysrepo load-modules 
  yang commands generated successfully for module=ietf-interfaces
  yang commands generated successfully for module=ietf-system
  yang commands generated successfully for module=ietf-access-control-list
  yang commands generated successfully for module=ietf-routing
router# configure terminal 
router(config)# interfaces 
router(config-interfaces)# interface eth0
router(config-interface[eth0])# enabled true
router(config-interface[eth0])# commit
[INF] There are no subscribers for changes of the module "ietf-interfaces" in running DS.
 changes applied successfully!
router(config-interface[eth0])# description "WAN INET"
router(config-interface[eth0])# commit 
[INF] There are no subscribers for changes of the module "ietf-interfaces" in running DS.
 changes applied successfully!
router(config-interface[eth0])# print local-candidate-config format json
{
  "ietf-interfaces:interfaces": {
    "interface": [      
      {
        "name": "eth0",
        "description": "WAN INET",
        "type": "iana-if-type:other",
        "enabled": true
      }
    ]
  }
}

router(config-interface[eth0])# 
```

[+] to modify installed modules in sysrepo:
```commandline
router# sysrepo set-module-path /home/ali/CLionProjects/xain/DentOS/okda/onm_cli/yang/standard/ietf
router# sysrepo install-module ietf-vrrp.yang
[ERR] Failed to create input handler for "/home/ali/CLionProjects/xain/DentOS/okda/onm_cli/yang/standard/ietf��U/ietf-vrrp.yang".
pcrouter# sysrepo set-module-path /home/ali/CLionProjects/xain/DentOS/okda/onm_cli/yang/standard/ietf/RFC
pcrouter# sysrepo install-module ietf-vrrp.yang
[INF] Module "ietf-vrrp" was installed.
[INF] File "ietf-vrrp@2018-03-13.yang" was installed.
router# sysrepo list-modules
[+] ietf-ipv4-unicast-routing
[+] ietf-routing
[+] ietf-vrrp
router#
```