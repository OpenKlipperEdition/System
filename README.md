# NebulaOS Kernel

Linux 6.6 kernel SDK used by [NebulaOS](https://github.com/coreflake1/NebulaOS) on the Creality
Ender-3 V3 KE / Ingenic X2000 platform.

> **The `openke` branch name is historical.** `NebulaOS-kernel` is a NebulaOS component and is
> separate from the [OpenKE](https://github.com/coreflake1/guppyscreen) project — the branch predates
> this repo's rename and kept its original name rather than being renamed cosmetically for branding.

| Branch | Role |
|---|---|
| `openke` | **Canonical NebulaOS lineage.** Every kernel-source change NebulaOS made — NS2009 touch, the display panel driver, BT H5 vendor extension, a watchdog fix, DTS wiring, `arch/mips/Kconfig` compression selects — lives here as a real, reviewable commit, rather than as a patch file applied at build time. |
| `main` | Tracks the original upstream SDK ([`Llixuma/ingenic-linux-kernel6.6-x2000-v1.0-20250221`](https://github.com/Llixuma/ingenic-linux-kernel6.6-x2000-v1.0-20250221)) unmodified — this fork's base, not touched by NebulaOS work. |

## This usually isn't the repo you want to build directly

[`NebulaOS-firmware`](https://github.com/coreflake1/NebulaOS-firmware) pins an exact commit of this
repo's `openke` branch (`KERNEL_PIN` in `manifests/dependencies.conf`), fetches it as a sparse
checkout (`kernel/kernel-6.6` only — the full SDK is ~684MB), and applies 8 accepted build-time kernel
variants on top before compiling. If you want the complete NebulaOS OS image, start at
`NebulaOS-firmware`, not here.

## The 8 accepted build-time variants

Tracked and applied by `NebulaOS-firmware`'s `scripts/build/apply-qualified-baseline.sh`, in this
order (no inter-script ordering dependency — each owns disjoint files or a uniquely-marked
append-only region):

1. `preempt-variant.sh` (R1) — PREEMPT_RT
2. `wifi-sdio-variant.sh` (W3) — WiFi SDIO IRQ priority
3. `display-vsync-variant.sh` (V1) — VSYNC-gated pan
4. `pinctrl-ownership-fix-variant.sh` (FIX1)
5. `backlight-final-controller-variant.sh` (FINAL1)
6. `pwm-state-readback-variant.sh` (GETSTATE1)
7. `touch-final-qualification-variant.sh` (FINALQUAL1)
8. `wifi-roamoff-disable-variant.sh` (ROAMOFF1) — disables brcmfmac firmware roaming

These live in `NebulaOS-firmware`, not this repo — this repo only holds the base kernel source they
patch at build time.

## License

This is a multi-component SDK (kernel, Buildroot, u-boot, and several vendored external tools),
each under its own upstream license — there is no single root `LICENSE` file because no single
license covers everything here. The Linux kernel itself is at `kernel/kernel-6.6/COPYING` (GPL-2.0)
and `kernel/kernel-6.6/LICENSES/`; other top-level directories carry their own `COPYING`/`LICENSE`
files. GitHub's automatic license detector only scans the repo root, so it correctly shows no
detected license for this repo as a whole — check the specific subdirectory you're using instead.
