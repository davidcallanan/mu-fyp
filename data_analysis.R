# Disclaimer: This script was entirely AI-generated, with some guidance on what plots were desired.

library(ggplot2)
library(dplyr)
library(patchwork)

# --- Data ---
raw <- list(
  "PIT"        = list(A = c(49163700, 32232100, 23710900, 10881700, 44196800, 35658300, 28627000, 38259500, 35749500, 27882300),
                      B = c(37108800, 77806900,  9941500, 50743900, 40544400, 47949300, 24032700, 28140800, 17049300, 48174300)),
  "RTC"        = list(A = c(10623000, 13767200, 10144100, 12396000, 13327600, 12964600,  8971100, 13359500, 12203400, 14333100),
                      B = c(24012200, 23249400, 23354000, 25870600, 24965800, 23079700, 21560600, 25708000, 20770900, 23185700)),
  "I/O Wait"   = list(A = c(154751400, 159377100, 120183500, 163299900, 157758500, 167298800, 133652700, 161642200, 134500000, 170489200),
                      B = c(321763300, 287924800, 303820800, 318400500, 306523700, 270913000, 289277200, 316938000, 309212500, 222781600)),
  "VGA Cursor" = list(A = c(39006700, 44255600, 40318000, 41454600, 53536100, 53156900, 48901400, 48509900, 42772800, 51832600),
                      B = c(65824600, 76507800, 76798000, 80639600, 83223000, 74050200, 82686200, 84997100, 85976300, 61198300))
)

bm_levels <- c("PIT", "RTC", "I/O Wait", "VGA Cursor")

# --- Tidy ---
df <- bind_rows(lapply(names(raw), function(bm) {
  bind_rows(
    tibble(benchmark = bm, impl = "A", value = raw[[bm]]$A),
    tibble(benchmark = bm, impl = "B", value = raw[[bm]]$B)
  )
})) |>
  mutate(
    value_k   = (value / 60) / 1e3,
    benchmark = factor(benchmark, levels = bm_levels),
    impl      = factor(impl)
  )

impl_colours <- c("A" = "#4E79A7", "B" = "#F28E2B")

theme_clean <- function() {
  theme_minimal(base_size = 13) +
    theme(
      plot.title         = element_text(face = "bold", hjust = 0.5, size = 15),
      panel.grid.minor   = element_blank(),
      panel.grid.major.x = element_blank(),
      axis.title         = element_text(colour = "grey30"),
      axis.text          = element_text(colour = "grey20"),
      plot.background    = element_rect(fill = "white", colour = NA)
    )
}

# =============================================================================
# PAGE 1: Raw data box + strip plots
# =============================================================================
make_box_plot <- function(bm_name) {
  d     <- filter(df, benchmark == bm_name)
  d_sum <- d |> group_by(impl) |> summarise(mean_k = mean(value_k), .groups = "drop")
  
  ggplot() +
    geom_boxplot(
      data  = d,
      aes(x = impl, y = value_k, fill = impl),
      width = 0.4, alpha = 0.3, outlier.shape = NA, colour = "grey40"
    ) +
    geom_jitter(
      data  = d,
      aes(x = impl, y = value_k, colour = impl),
      width = 0.08, size = 2.5, alpha = 0.85
    ) +
    geom_point(
      data  = d_sum,
      aes(x = impl, y = mean_k),
      shape = 18, size = 4, colour = "black"
    ) +
    scale_fill_manual(values = impl_colours, guide = "none") +
    scale_colour_manual(values = impl_colours, guide = "none") +
    labs(title = bm_name, x = "Implementation", y = "Iterations/s (thousands)") +
    theme_clean()
}

box_plots <- lapply(bm_levels, make_box_plot)

page1 <- (box_plots[[1]] | box_plots[[2]]) / (box_plots[[3]] | box_plots[[4]]) +
  plot_annotation(
    title = "Benchmark Comparison: Implementation A vs B",
    theme = theme(plot.title = element_text(face = "bold", size = 18, hjust = 0.5))
  )

# =============================================================================
# PAGE 2: Coefficient of Variation
# =============================================================================
df_cv <- df |>
  group_by(benchmark, impl) |>
  summarise(
    cv = (sd(value_k) / mean(value_k)) * 100,  # as a percentage
    .groups = "drop"
  )

page2 <- ggplot(df_cv, aes(x = benchmark, y = cv, fill = impl)) +
  geom_col(position = position_dodge(width = 0.6), width = 0.5, alpha = 0.85) +
  geom_text(
    aes(label = paste0(round(cv, 1), "%")),
    position = position_dodge(width = 0.6),
    vjust = -0.5, size = 3.8, colour = "grey20"
  ) +
  scale_fill_manual(values = impl_colours, name = "Implementation") +
  scale_y_continuous(
    expand = expansion(mult = c(0, 0.12)),
    labels = function(x) paste0(x, "%")
  ) +
  labs(
    title = "Coefficient of Variation by Benchmark",
    x     = "Benchmark",
    y     = "CV (σ / mean)"
  ) +
  theme_minimal(base_size = 13) +
  theme(
    plot.title         = element_text(face = "bold", hjust = 0.5, size = 18),
    panel.grid.minor   = element_blank(),
    panel.grid.major.x = element_blank(),
    axis.title         = element_text(colour = "grey30"),
    axis.text          = element_text(colour = "grey20"),
    legend.position    = "top",
    plot.background    = element_rect(fill = "white", colour = NA)
  )

# =============================================================================
# PAGE 3: One-sided Welch's t-test (H1: B > A) per benchmark
# =============================================================================
test_results <- bind_rows(lapply(bm_levels, function(bm) {
  a  <- df |> filter(benchmark == bm, impl == "A") |> pull(value_k)
  b  <- df |> filter(benchmark == bm, impl == "B") |> pull(value_k)
  tt <- tryCatch(
    t.test(b, a, alternative = "greater", var.equal = FALSE),
    error = function(e) NULL
  )
  pooled_sd <- sqrt((sd(a)^2 + sd(b)^2) / 2)
  cohens_d  <- if (!is.na(pooled_sd) && pooled_sd > 0) (mean(b) - mean(a)) / pooled_sd else NA
  tibble(
    Benchmark  = bm,
    n_A        = length(a),
    n_B        = length(b),
    mean_A     = round(mean(a), 2),
    mean_B     = round(mean(b), 2),
    t_stat     = if (!is.null(tt)) round(tt$statistic, 3) else NA,
    df_welch   = if (!is.null(tt)) round(tt$parameter, 2) else NA,
    p_value    = if (!is.null(tt)) round(tt$p.value, 4) else NA,
    cohens_d   = round(cohens_d, 3),
    conclusion = case_when(
      is.na(p_value) ~ "insufficient data",
      p_value < 0.05 ~ "B > A (significant)",
      TRUE           ~ "no significant difference"
    )
  )
}))

cat("\n=== One-sided Welch's t-test: H0: B <= A,  H1: B > A ===\n\n")
print(as.data.frame(test_results), row.names = FALSE)
page3 <- ggplot(test_results, aes(x = factor(Benchmark, levels = bm_levels),
                                  y = -log10(p_value), fill = conclusion)) +
  geom_col(width = 0.5, alpha = 0.85) +
  geom_hline(yintercept = -log10(0.05), linetype = "dashed",
             colour = "red", linewidth = 0.8) +
  annotate("text", x = 0.5, y = -log10(0.05) + 0.15, label = "α = 0.05",
           hjust = 0, colour = "red", size = 3.5) +
  geom_text(
    aes(label = ifelse(p_value < 0.0001, "p < 0.0001",
                       paste0("p = ", round(p_value, 4)))),
    vjust = -0.5, size = 3.5, colour = "grey20"
  ) +
  scale_y_continuous(expand = expansion(mult = c(0, 0.15))) +
  scale_fill_manual(
    values = c(
      "B > A (significant)"       = "#2ca02c",
      "no significant difference" = "#d62728",
      "insufficient data"         = "grey60"
    ),
    name = ""
  ) +
  labs(
    title = "One-Sided Welch's t-test: Is B Better Than A?",
    x     = "Benchmark",
    y     = expression(-log[10](p))
  ) +
  theme_minimal(base_size = 13) +
  theme(
    plot.title       = element_text(face = "bold", hjust = 0.5, size = 15),
    panel.grid.minor = element_blank(),
    legend.position  = "top",
    plot.background  = element_rect(fill = "white", colour = NA)
  )

# =============================================================================
# Save
# =============================================================================
ggsave("benchmarks_page1_raw.png",        page1, width = 12, height = 9, dpi = 200)
ggsave("benchmarks_page2_cv.png",         page2, width = 10, height = 6, dpi = 200)
ggsave("benchmarks_page3_hypothesis.png", page3, width = 10, height = 6, dpi = 200)

print(page1)
print(page2)
print(page3)

df_forest <- bind_rows(lapply(bm_levels, function(bm) {
  a <- df |> filter(benchmark == bm, impl == "A") |> pull(value_k)
  b <- df |> filter(benchmark == bm, impl == "B") |> pull(value_k)
  
  tt <- tryCatch(
    t.test(b, a, var.equal = FALSE),
    error = function(e) NULL
  )
  
  mean_diff_pct    <- ((mean(b) - mean(a)) / mean(a)) * 100
  ci_lo_pct <- if (!is.null(tt)) ((tt$conf.int[1]) / mean(a)) * 100 else NA
  ci_hi_pct <- if (!is.null(tt)) ((tt$conf.int[2]) / mean(a)) * 100 else NA
  
  tibble(
    benchmark    = bm,
    gain_pct     = mean_diff_pct,
    ci_lo        = ci_lo_pct,
    ci_hi        = ci_hi_pct
  )
})) |>
  mutate(
    benchmark = factor(benchmark, levels = rev(bm_levels)),
    colour    = ifelse(ci_lo > 0, "#2ca02c", "#d62728")
  )

page4 <- ggplot(df_forest, aes(x = gain_pct, y = benchmark)) +
  # shaded "no gain" region
  annotate("rect", xmin = -Inf, xmax = 0, ymin = -Inf, ymax = Inf,
           fill = "#d62728", alpha = 0.04) +
  annotate("rect", xmin = 0, xmax = Inf, ymin = -Inf, ymax = Inf,
           fill = "#2ca02c", alpha = 0.04) +
  # zero line
  geom_vline(xintercept = 0, linetype = "solid", colour = "grey40", linewidth = 0.8) +
  # confidence intervals
  geom_errorbarh(
    aes(xmin = ci_lo, xmax = ci_hi),
    height = 0.15, linewidth = 0.9, colour = "grey30"
  ) +
  # point estimate
  geom_point(aes(colour = colour), size = 4) +
  # label the point estimate
  geom_text(
    aes(label = paste0(ifelse(gain_pct > 0, "+", ""), round(gain_pct, 1), "%")),
    vjust = -1.1, size = 3.8, colour = "grey20"
  ) +
  scale_colour_identity() +
  scale_x_continuous(labels = function(x) paste0(x, "%")) +
  labs(
    title = "Performance Gain of B over A with 95% Confidence Interval",
    x     = "Mean % improvement (B over A)",
    y     = NULL
  ) +
  theme_minimal(base_size = 13) +
  theme(
    plot.title       = element_text(face = "bold", hjust = 0.5, size = 15),
    panel.grid.minor = element_blank(),
    panel.grid.major.y = element_blank(),
    axis.text.y      = element_text(size = 13, colour = "grey20"),
    axis.title       = element_text(colour = "grey30"),
    plot.background  = element_rect(fill = "white", colour = NA)
  )

ggsave("benchmarks_page4_forest.png", page4, width = 10, height = 5, dpi = 200)
print(page4)

