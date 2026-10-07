import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import pandas as pd
import requests


def gerar_dashboard_premium_final():
    url_da_sua_api = "https://api-irrigacao.online/api/telemetry"

    print("Renderizando dashboard de alta fidelidade...")

    try:
        response = requests.get(url_da_sua_api)
        response.raise_for_status()
        dados_json = response.json()
    except Exception as e:
        print(f"Erro ao conectar na API: {e}")
        return

    # Tratamento de dados analíticos
    df = pd.DataFrame(dados_json)
    df["criado_em"] = pd.to_datetime(
        df["criado_em"], format="%Y-%m-%d %H:%M:%S", errors="coerce"
    )
    df = df.dropna(subset=["criado_em"]).sort_values(by="criado_em")

    # Armazena o ID do sensor antes de alterar o índice (Corrigido com .iloc[0])
    if "sensor_id" in df.columns and not df.empty:
        sensor_name = str(df["sensor_id"].iloc[0]).upper()
    else:
        sensor_name = "ESP32_MULTISENSOR_01"

    df.set_index("criado_em", inplace=True)
    df_resampled = df.resample("30min").mean(numeric_only=True).dropna(subset=["temperatura"])

    if df_resampled.empty:
        print("Nenhum dado processado.")
        return

    # Interface Dark de Alta Fidelidade
    plt.style.use("dark_background")
    fig, axs = plt.subplots(2, 2, figsize=(16, 10), sharex=True)

    # Customização de cores de fundo (Fosco Premium)
    fig.patch.set_facecolor("#111116")
    
    # Paleta Neon Suave Executiva
    cores = {
        "temp": "#ff5252",  # Vermelho Vivo
        "umid": "#00b0ff",  # Ciano Elétrico
        "pres": "#ffab00",  # Âmbar Ouro
        "rad": "#00e676",  # Verde Esmeralda
    }

    # Configuração global dos quadrantes para visual clean
    for ax in axs.flat:
        ax.set_facecolor("#17171e")
        ax.grid(True, color="#262632", linestyle=":", linewidth=0.6)
        ax.tick_params(colors="#8e8e9f", labelsize=10)
        
        # O SEGREDO VISUAL: Permite que o eixo Y flutue próximo aos valores reais
        ax.set_autoscale_on(True) 
        
        # Remove bordas para visual minimalista moderno
        for spine in ["top", "right", "left", "bottom"]:
            ax.spines[spine].set_visible(False)

    # Título Principal Texturizado Corporativo
    fig.suptitle(
        f"TELEMETRIA ANALÍTICA DE HARDWARE\nSENSOR ID: {sensor_name}",
        fontsize=16,
        color="#ffffff",
        fontweight="bold",
        y=0.96,
        fontfamily="sans-serif",
    )

    # --- Quadrante 1: Temperatura ---
    axs[0, 0].plot(df_resampled.index, df_resampled["temperatura"], color=cores["temp"], linewidth=2.5, alpha=0.9)
    axs[0, 0].fill_between(df_resampled.index, df_resampled["temperatura"], df_resampled["temperatura"].min() - 0.5, alpha=0.06, color=cores["temp"])
    axs[0, 0].set_title("TEMPERATURA AMBIENTAL (°C)", color="#e0e0e6", fontsize=11, fontweight="bold", pad=12)

    # --- Quadrante 2: Umidade ---
    axs[0, 1].plot(df_resampled.index, df_resampled["umidade"], color=cores["umid"], linewidth=2.5, alpha=0.9)
    axs[0, 1].fill_between(df_resampled.index, df_resampled["umidade"], df_resampled["umidade"].min() - 2, alpha=0.06, color=cores["umid"])
    axs[0, 1].set_title("UMIDADE RELATIVA DO AR (%)", color="#e0e0e6", fontsize=11, fontweight="bold", pad=12)

    # --- Quadrante 3: Pressão ---
    axs[1, 0].plot(df_resampled.index, df_resampled["pressao"], color=cores["pres"], linewidth=2.5, alpha=0.9)
    axs[1, 0].fill_between(df_resampled.index, df_resampled["pressao"], df_resampled["pressao"].min() - 1, alpha=0.04, color=cores["pres"])
    axs[1, 0].set_title("PRESSÃO ATMOSFÉRICA (hPa)", color="#e0e0e6", fontsize=11, fontweight="bold", pad=12)

    # --- Quadrante 4: Irradiação Solar ---
    axs[1, 1].plot(df_resampled.index, df_resampled["irradiacao_solar"], color=cores["rad"], linewidth=2.5, alpha=0.9)
    axs[1, 1].fill_between(df_resampled.index, df_resampled["irradiacao_solar"], df_resampled["irradiacao_solar"].min() - 10, alpha=0.06, color=cores["rad"])
    axs[1, 1].set_title("IRRADIAÇÃO SOLAR INCIDENTE (W/m²)", color="#e0e0e6", fontsize=11, fontweight="bold", pad=12)

    # Otimização Fina de Datas no Eixo X (Design Linear Horizontal)
    for ax in axs[1, :]:
        ax.xaxis.set_major_formatter(mdates.DateFormatter("%d/%m\n%H:%M"))
        ax.xaxis.set_major_locator(mdates.AutoDateLocator(minticks=5, maxticks=7))
        ax.tick_params(axis="x", rotation=0, colors="#8e8e9f")

    # Ajuste milimétrico de bordas
    plt.tight_layout(rect=[0.02, 0.02, 0.98, 0.92])

    print("Exibindo painel executivo premium...")
    plt.show()


if __name__ == "__main__":
    gerar_dashboard_premium_final()
