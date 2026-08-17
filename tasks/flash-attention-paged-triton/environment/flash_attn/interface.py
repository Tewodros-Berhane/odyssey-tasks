import torch
from torch.autograd import Function
import math

class _FlashAttentionFunction(Function):
    @staticmethod
    def forward(ctx, q, k, v, causal=True, sm_scale=None):
        if sm_scale is None:
            sm_scale = 1.0 / math.sqrt(q.shape[-1])

        # Baseline PyTorch implementation for starter environment
        # To be replaced with custom Triton kernel
        if causal:
            L, S = q.size(-2), k.size(-2)
            attn_mask = torch.ones(L, S, dtype=torch.bool, device=q.device).tril(diagonal=0)
            out = torch.nn.functional.scaled_dot_product_attention(
                q, k, v, attn_mask=attn_mask, scale=sm_scale
            )
        else:
            out = torch.nn.functional.scaled_dot_product_attention(
                q, k, v, scale=sm_scale
            )

        ctx.save_for_backward(q, k, v, out)
        ctx.causal = causal
        ctx.sm_scale = sm_scale
        return out

    @staticmethod
    def backward(ctx, dout):
        q, k, v, out = ctx.saved_tensors
        # Numerical autograd reference for backward
        q_req = q.detach().requires_grad_(True)
        k_req = k.detach().requires_grad_(True)
        v_req = v.detach().requires_grad_(True)

        with torch.enable_grad():
            ref_out = torch.nn.functional.scaled_dot_product_attention(
                q_req, k_req, v_req, is_causal=ctx.causal, scale=ctx.sm_scale
            )
            dq, dk, dv = torch.autograd.grad(ref_out, (q_req, k_req, v_req), grad_outputs=dout)

        return dq, dk, dv, None, None

def flash_attention_func(q, k, v, causal=True, sm_scale=None):
    return _FlashAttentionFunction.apply(q, k, v, causal, sm_scale)

def paged_flash_attention_func(q, k_cache, v_cache, block_tables, seq_lens, sm_scale=None):
    # Paged KV Cache interface
    batch_size, num_heads, head_dim = q.shape[0], q.shape[1], q.shape[2]
    # SKELETON: To be implemented in Triton
    out = torch.zeros_like(q)
    return out
