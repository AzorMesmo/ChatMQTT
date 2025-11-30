# ChatMQTT

Mateus Azor Frutuoso,\
Thailha Roanni Schimit Cavalheiro.

## Compilação/Excecução

**Comando Para Compilação:** "gcc main.c publisher.c subscriber.c agent.c messages.c -o main -lpaho-mqtt3as -pthread".

**Comando Para Excecução:** "./main".

## Debbug

**LIMPAR TUDO**
- "sudo systemctl stop mosquitto"
- "sudo rm /var/lib/mosquitto/mosquitto.db"
- "sudo systemctl start mosquitto"