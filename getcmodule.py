import ctypes
def get_c(lib_path):
    # Cの関数を取得
    # lib_path =  os.path.join(os.path.dirname(__file__), "libservo.so") # 共有ライブラリのパス
    lib_C = ctypes.CDLL(lib_path) # Cの共有ライブラリをロード
    # my_clib.add_numbers.argtypes = [ctypes.c_int, ctypes.c_int]
    
    lib_C.main.restype = ctypes.c_int # 戻り値の型を指定
    return lib_C

if __name__ == "__main__":
    get_c()