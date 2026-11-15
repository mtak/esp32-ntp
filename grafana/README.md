# Grafana dashboard

[esp32-ntp-dashboard.json](esp32-ntp-dashboard.json) is an importable Grafana dashboard for this firmware's Prometheus metrics. It shows clock status and accuracy, oscillator drift, GNSS reception and satellite geometry, NTP reply timing, Ethernet and device health, and optional RTC/TCXO measurements.

1. Configure Prometheus to scrape the device's `/metrics` endpoint. For the default management port:

   ```yaml
   scrape_configs:
     - job_name: esp32-ntp
       scrape_interval: 15s
       static_configs:
         - targets: ['DEVICE_IP:8080']
   ```

2. Add Prometheus as a Grafana data source, then use **Dashboards → New → Import** to upload the JSON file. Select the Prometheus data source and device at the top of the dashboard. The `instance` selector uses the scrape target label.

The fit residual panel is labeled separately from clock offset because `ntp_rms_offset_seconds` includes oscillator frequency ramp. TCXO series are hidden until `ntp_tcxo_valid` is 1. Reply-path panels remain at zero until the device serves NTP requests. DHCP event panels may stay at zero with a static address. The scrape-health panel uses Prometheus's `up` metric, so it can show a failed scrape even when firmware metrics disappear.

The reply-time percentile panel uses cumulative bucket counters. The firmware declares them as counters because it does not export the `_sum` and `_count` series required for a full Prometheus histogram. Its quantiles are estimates bounded by the available bucket edges.

The reset-reason labels use the ESP-IDF `esp_reset_reason_t` enum.
