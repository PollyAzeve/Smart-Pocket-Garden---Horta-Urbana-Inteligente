import time
import requests
from concurrent.futures import ThreadPoolExecutor, as_completed

url = "https://api-irrigacao.online/api/telemetry"
TOTAL_REQUISICOES = 7000
MAX_WORKERS = 20  # Número de requisições paralelas simultâneas

print(f"⚡ INICIANDO TESTE PARALELO DE ALTA FREQUÊNCIA ({TOTAL_REQUISICOES} REQUISIÇÕES)")
print(f"Configurado com {MAX_WORKERS} threads simultâneas...\n")

# Criamos uma sessão para REUTILIZAR as conexões HTTP (Keep-Alive)
session = requests.Session()

tempos = []
sucessos = 0
falhas = 0

def disparar_requisicao(id_req):
    try:
        inicio = time.perf_counter()
        # Usando a sessão reaproveitável em vez de requests.get direto
        resposta = session.get(url, timeout=5)
        fim = time.perf_counter()
        duracao = fim - inicio
        
        if resposta.status_code == 200:
            dados = resposta.json()
            ultimo_registro = dados[0] if isinstance(dados, list) and len(dados) > 0 else {}
            return {"status": "sucesso", "tempo": duracao, "id_req": id_req, "dados": ultimo_registro}
        else:
            return {"status": "erro_http", "codigo": resposta.status_code, "id_req": id_req}
            
    except requests.exceptions.RequestException:
        return {"status": "falha_rede", "id_req": id_req}

# Gerencia o disparo paralelo usando Threads
tempo_inicio_total = time.perf_counter()
with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
    # Agenda todas as 200 requisições
    tarefas = [executor.submit(disparar_requisicao, i) for i in range(1, TOTAL_REQUISICOES + 1)]
    
    # Processa os resultados conforme eles vão terminando
    for cont, tarefa in enumerate(as_completed(tarefas), 1):
        resultado = tarefa.result()
        
        if resultado["status"] == "sucesso":
            sucessos += 1
            tempos.append(resultado["tempo"])
            
            # Print resumido a cada 10 ou na primeira/última
            if cont % 10 == 0 or cont == 1 or cont == TOTAL_REQUISICOES:
                id_atual = resultado["dados"].get("id", "N/A")
                temp_atual = resultado["dados"].get("temperatura", "N/A")
                print(f"🏃 Terminou #{cont:03d} (Req Orig #{resultado['id_req']:03d}): Tempo: {resultado['tempo']:.3f}s | ID: {id_atual} | Temp: {temp_atual}°C")
        else:
            falhas += 1
            if resultado["status"] == "erro_http":
                print(f"❌ Req #{resultado['id_req']:03d}: ERRO HTTP {resultado['codigo']}")
            else:
                print(f"💥 Req #{resultado['id_req']:03d}: FALHA CRÍTICA DE REDE")

tempo_fim_total = time.perf_counter()

# ---------------------------------------------------------------------------
# RELATÓRIO FINAL
# ---------------------------------------------------------------------------
print("\n" + "="*45)
print("📊 RELATÓRIO DE VELOCIDADE (PARALELIZADO)")
print("="*45)
print(f"• Requisições processadas: {TOTAL_REQUISICOES}")
print(f"• Sucessos (200 OK): {sucessos}")
print(f"• Falhas do Servidor: {falhas}")

if tempos:
    tempo_medio = sum(tempos) / len(tempos)
    print(f"• Tempo Médio por requisição: {tempo_medio:.3f} segundos")
    print(f"• Resposta mais RÁPIDA: {min(tempos):.3f} segundos")
    print(f"• Resposta mais LENTA: {max(tempos):.3f} segundos")
    print(f"• Tempo de Execução do Script: {tempo_fim_total - tempo_inicio_total:.2f} segundos")
print("="*45)
