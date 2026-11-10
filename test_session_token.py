from pathlib import Path

import session_token


def test_secret_is_usable_normal():
    assert session_token.secret_is_usable("0123456789abcdef")
    assert session_token.secret_is_usable("native-test-session-hmac-secret")


def test_secret_is_usable_rejects_missing_and_short():
    assert not session_token.secret_is_usable(None)
    assert not session_token.secret_is_usable("")
    assert not session_token.secret_is_usable("short-secret")
    assert not session_token.secret_is_usable("0123456789abcde")


def test_secret_is_usable_rejects_placeholder():
    assert not session_token.secret_is_usable("CHANGE_ME")
    assert not session_token.secret_is_usable("CHANGE_ME_SESSION_HMAC_SECRET")


def test_secret_is_usable_counts_bytes_like_firmware():
    # Firmware sessionSecretIsUsable uses strlen (bytes), not characters.
    assert session_token.secret_is_usable("あ" * 8)  # 8 chars, 24 bytes
    assert not session_token.secret_is_usable("あ" * 5)  # 15 bytes


def test_readme_testpub_example_is_not_placeholder():
    readme = Path(__file__).resolve().parent / "readme.md"
    text = readme.read_text(encoding="utf-8")
    assert "export SESSION_HMAC_SECRET='CHANGE_ME_SESSION_HMAC_SECRET'" not in text
    assert "export SESSION_HMAC_SECRET='<config.h と同じ実値>'" in text


if __name__ == "__main__":
    test_secret_is_usable_normal()
    test_secret_is_usable_rejects_missing_and_short()
    test_secret_is_usable_rejects_placeholder()
    test_secret_is_usable_counts_bytes_like_firmware()
    test_readme_testpub_example_is_not_placeholder()
    print("all tests passed")
