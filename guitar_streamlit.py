import streamlit as st
import mqttpub
topic = "guitar/code"

if "client" not in st.session_state: # 呼び出しは一回だけです
    st.session_state.client = mqttpub.publish_init()
    st.success("mqtt接続完了")
    
def send(message): # mqtt送信
    mqttpub.publish(st.session_state.client, topic, message)
    st.success(f"[{message}]を送信")
    
def main():
    # 配列を２列
    major ,minor = st.columns(2)
    # メジャー設定
    major_triad = "Mt"
    text_major_triad = "メジャー"
    
    # マイナー設定
    minor_triad = "mt"
    text_minor_triad = "マイナー"
       
    st.markdown( # ボタンのスタイル
        """
        <style>
        .stButton > button {
            font-size: 25px;
            height: 100px;
            width: 100px;
            padding: 10px 10px;
        }
        </style>
        """,
        unsafe_allow_html=True
    )
    # メジャーコード
    with major:
        with st.container(horizontal=True, horizontal_alignment="left"): # 左置き
            cMt = st.button(f"C{text_major_triad}")
            aMt = st.button(f"A{text_major_triad}")
            eMt = st.button(f"E{text_major_triad}")
            fMt = st.button(f"F{text_major_triad}")
            gMt = st.button(f"G{text_major_triad}")
            dMt = st.button(f"D{text_major_triad}")
    
    # マイナーコード
    with minor:
        with st.container(horizontal=True, horizontal_alignment="right"): # 右置き
            cmt = st.button(f"C{text_minor_triad}")
            amt = st.button(f"A{text_minor_triad}")
            emt = st.button(f"E{text_minor_triad}")
            fmt = st.button(f"F{text_minor_triad}")
            gmt = st.button(f"G{text_minor_triad}")

    # メジャーコード
    if cMt:
        send(f"c{major_triad}")
        return 0
    elif aMt:
        send(f"a{major_triad}")
        return 0
    elif eMt:
        send(f"e{major_triad}")
        return 0
    elif fMt:
        send(f"f{major_triad}")
        return 0
    elif gMt:
        send(f"g{major_triad}")
        return 0
    elif dMt:
        send(f"d{major_triad}")
                
    # マイナーコード
    if cmt:
        send(f"c{minor_triad}")
    elif amt:
        send(f"a{minor_triad}")
        return 0
    elif emt:
        send(f"e{minor_triad}")
        return 0
    elif fmt:
        send(f"f{minor_triad}")
        return 0
    elif gmt:
        send(f"g{minor_triad}")
        return 0
    
    
if __name__ == "__main__":
    main()