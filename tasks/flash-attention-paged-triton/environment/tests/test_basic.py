import torch
import pytest
from flash_attn import flash_attention_func

def test_flash_attn_forward_shape():
    batch, heads, seqlen, dim = 2, 4, 128, 64
    q = torch.randn(batch, heads, seqlen, dim, device="cpu", dtype=torch.float32)
    k = torch.randn(batch, heads, seqlen, dim, device="cpu", dtype=torch.float32)
    v = torch.randn(batch, heads, seqlen, dim, device="cpu", dtype=torch.float32)

    out = flash_attention_func(q, k, v, causal=True)
    assert out.shape == q.shape
    print("test_flash_attn_forward_shape passed!")

if __name__ == "__main__":
    test_flash_attn_forward_shape()
