# figures.R
# Generate all figures for KineticHeap++ paper
# Usage: Rscript analysis/figures.R

# Install required packages if needed
if (!require(ggplot2))  install.packages("ggplot2",  repos="https://cran.r-project.org")
if (!require(dplyr))    install.packages("dplyr",    repos="https://cran.r-project.org")
if (!require(scales))   install.packages("scales",   repos="https://cran.r-project.org")

library(ggplot2)
library(dplyr)
library(scales)

dir.create("results/figures", showWarnings = FALSE)

# ─── Load Data ────────────────────────────────────────────
kh   <- read.csv("results/scaling.csv")
naive  <- read.csv("results/baseline_naive.csv")
idx    <- read.csv("results/baseline_indexed.csv")
stl    <- read.csv("results/baseline_stl.csv")

# ─── Figure 1: KineticHeap++ Scaling (log-log) ────────────
png("results/figures/fig1_scaling.png", width=800, height=600)
ggplot(kh, aes(x=n, y=cert_count)) +
  geom_line(color="#2196F3", linewidth=1.2) +
  geom_point(color="#2196F3", size=3) +
  scale_x_log10(labels=comma) +
  scale_y_log10(labels=comma) +
  labs(
    title    = "KineticHeap++ Certificate Scaling",
    subtitle = "Log-log scale — random linear-motion workload",
    x        = "n (number of elements)",
    y        = "Total certificate count"
  ) +
  theme_minimal(base_size=14)
dev.off()
cat("Saved: fig1_scaling.png\n")

# ─── Figure 2: Runtime Comparison ─────────────────────────
common_n <- c(10000, 30000, 100000)

kh_sub  <- kh  %>% filter(n %in% common_n) %>% mutate(method="KineticHeap++")
idx_sub <- idx %>% filter(n %in% common_n) %>% mutate(method="Indexed Heap (B4)")
stl_sub <- stl %>% filter(n %in% common_n) %>% mutate(method="STL Heap (B8)")

combined <- bind_rows(kh_sub, idx_sub, stl_sub)

png("results/figures/fig2_comparison.png", width=900, height=600)
ggplot(combined, aes(x=factor(n), y=time_sec, fill=method)) +
  geom_bar(stat="identity", position="dodge") +
  scale_fill_manual(values=c(
    "KineticHeap++"    = "#4CAF50",
    "Indexed Heap (B4)"= "#FF9800",
    "STL Heap (B8)"    = "#F44336"
  )) +
  labs(
    title = "Runtime Comparison: KineticHeap++ vs Baselines",
    x     = "n (number of elements)",
    y     = "Wall-clock time (seconds)",
    fill  = "Method"
  ) +
  theme_minimal(base_size=14)
dev.off()
cat("Saved: fig2_comparison.png\n")

# ─── Figure 3: Speedup ────────────────────────────────────
speedup <- data.frame(
  n       = common_n,
  indexed = idx_sub$time_sec / kh_sub$time_sec,
  stl     = stl_sub$time_sec / kh_sub$time_sec
)

png("results/figures/fig3_speedup.png", width=800, height=600)
ggplot(speedup, aes(x=factor(n))) +
  geom_line(aes(y=indexed, group=1, color="vs Indexed Heap"),
            linewidth=1.2) +
  geom_point(aes(y=indexed, color="vs Indexed Heap"), size=3) +
  geom_line(aes(y=stl, group=1, color="vs STL Heap"),
            linewidth=1.2) +
  geom_point(aes(y=stl, color="vs STL Heap"), size=3) +
  scale_color_manual(values=c(
    "vs Indexed Heap" = "#FF9800",
    "vs STL Heap"     = "#F44336"
  )) +
  labs(
    title  = "KineticHeap++ Speedup over Baselines",
    x      = "n (number of elements)",
    y      = "Speedup (×)",
    color  = "Comparison"
  ) +
  theme_minimal(base_size=14)
dev.off()
cat("Saved: fig3_speedup.png\n")

# ─── Summary Statistics ───────────────────────────────────
cat("\n=== Summary ===\n")
cat("Max speedup over Indexed Heap:", 
    round(max(speedup$indexed), 1), "x\n")
cat("Max speedup over STL Heap:    ", 
    round(max(speedup$stl), 1), "x\n")
cat("\nAll figures saved to results/figures/\n")