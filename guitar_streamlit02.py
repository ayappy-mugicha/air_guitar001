import streamlit as st
import mqttpub

# --- 定数（変更の可能性が低い値をまとめておく） ---
TOPIC = "guitar/code"
TEXT_MAJOR = "メジャー"
TEXT_MINOR = "マイナー"


    # 1回だけMQTTクライアントを初期化
if "client" not in st.session_state:
    st.session_state.client = mqttpub.publish_init()
    st.success("MQTT接続完了 ✅")

def send(message):
    """MQTTでメッセージを送信する関数"""
    mqttpub.publish(st.session_state.client, TOPIC, message)
    st.success(f"[{message}]を送信 ✅")
    
def create_and_handle_buttons(col, chords, text_suffix, direction,message_prefix=""):
    """
    指定された列にボタンを動的に作成し、押されたときの処理を行う関数。
    
    Args:
        col (streamlit.column): ボタンを配置する列。
        chords (list): ボタンとして表示するコードのリスト。
        text_suffix (str): ボタンに表示されるテキストの接尾辞。
        message_prefix (str): MQTTメッセージに追加する接頭辞（例: マイナーコードの 'mt'）。
    """
    with col:
        
        # コードの数だけ列を作成
        cols = st.columns(len(chords))
        for i, chord in enumerate(chords):
            with cols[i]:
                with st.container(horizontal_alignment=direction):
                    # ボタンのテキストを動的に生成 (例: "Cメジャー")
                    button_text = f"{chord}{text_suffix}"
                    # ボタンのキーをユニークにするため、列名も使用
                    button_key = f"{col}_{button_text}"
                    
                    if st.button(button_text, key=button_key):
                        # MQTTメッセージを生成 (例: "c" または "dmt")
                        message = f"{chord.lower()}{message_prefix}"
                        send(message)

def main():
    """Streamlitアプリケーションのメイン関数"""
    
    # --- データ定義（コードを追加・変更する場合はここを編集） ---
    major_chords = ["C", "A", "E", "F", "G", "D"]
    minor_chords = ["D", "A", "E"]

    
    # ボタンのスタイル設定
    st.markdown(
        """
        <style>
        .stButton > button {
            font-size: 25px;
            height: 100px;
            width: 100px;
            padding: 10px;
        }
        </style>
        """,
        unsafe_allow_html=True
    )
    col1, col2 = st.columns(2)

    # レイアウトの作成
    major_col = st.columns(1)[0]
    minor_col = st.columns(1)[0]
    # # メジャーコードのボタンを作成
    # create_and_handle_buttons(major_col, major_chords, TEXT_MAJOR)
    
    # # マイナーコードのボタンを作成
    # create_and_handle_buttons(minor_col, minor_chords, TEXT_MINOR, "mt")
    
        # --- メジャーコードのセクション ---
    major_container = st.container()
    with major_container:
        with col1:
            create_and_handle_buttons(major_col, major_chords, TEXT_MAJOR ,"left")

    # --- マイナーコードのセクション ---
    minor_container = st.container()
    with minor_container:
        with col2:
            create_and_handle_buttons(minor_col, minor_chords, TEXT_MINOR, "right","m")
    
    # フルオープンボタン
    with st.container(horizontal=True, horizontal_alignment="center"):
        if st.button("full open"):
            send("open")
            # st.stop()

if __name__ == "__main__":
    main()