#!/usr/bin/env bash
#
# boot_proyecto.sh
# Lanza decoder.py, lora_pkt_fwd y app.py en paralelo usando tmux.

SESSION="lora_services"

# 1) Si la sesión ya existe, la matamos para reiniciar limpia
tmux has-session -t "$SESSION" 2>/dev/null && tmux kill-session -t "$SESSION"

# 2) Creamos nueva sesión en detached
tmux new-session -d -s "$SESSION" -n decoder \
  "cd ~/lora_webserver && echo '=== decoder.py ===' && python3 decoder.py; read"

# 3) Abrimos segunda ventana para el packet_forwarder
tmux new-window -t "$SESSION" -n pkt_fwd \
  "cd ~/sx1302_hal/packet_forwarder && echo '=== lora_pkt_fwd ===' && chmod +x lora_pkt_fwd && ./lora_pkt_fwd; read"

# 4) Tercera ventana para la app Flask
tmux new-window -t "$SESSION" -n app \
  "cd ~/lora_webserver && echo '=== app.py (Flask) ===' && python3 app.py; read"

# 5) Opcional: un panel index en la primera ventana
tmux split-window -h -t "$SESSION:decoder" \
  "echo 'Ventanas disponibles:'; tmux list-windows; read"

# 6) Adjuntar la sesión para verla en pantalla
tmux attach-session -t "$SESSION"
