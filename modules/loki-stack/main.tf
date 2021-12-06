locals {
  values = {
   loki = {
    "config" = {
      "auth_enabled" = false

      "compactor" = {
        "shared_store" = "s3"

        "working_directory" = "/data/loki/boltdb-shipper-compactor"
      }

      "ingester" = {
        "chunk_block_size" = 262144

        "chunk_idle_period" = "3m"

        "chunk_retain_period" = "1m"

        "lifecycler" = {
           "ring" = {
             "kvstore" = {
                "store" = "inmemory"
             }

             "replication_factor" = 1
          }
        }

        "max_transfer_retries" = 0
      }

      "limits_config" = {
        "enforce_metric_name" = false

        "reject_old_samples" = true

        "reject_old_samples_max_age" = "168h"
      }

      "schema_config" = {
        "configs" = [{
          "from" = "2020-08-05"

          "index" = {
            "period" = "24h"

            "prefix" = "index_"
          }

          "object_store" = "s3"

          "schema" = "v11"

          "store" = "boltdb-shipper"
        }]
      }

      "storage_config" = {
        "aws" = {
          "s3" = "https://${var.loki_bucket_access_key}:${var.loki_bucket_secret_key}@storage.yandexcloud.net/${var.loki_bucket_name}"

          "s3forcepathstyle" = true
        }

        "boltdb_shipper" = {
          "active_index_directory" = "/data/loki/boltdb-shipper-active"

          "cache_location" = "/data/loki/boltdb-shipper-cache"

          "cache_ttl" = "24h"

          "resync_interval" = "5s"

          "shared_store" = "s3"
        }
      }
    }
  }
 }
}
resource "helm_release" "loki-stack" {
  name              = "loki-stack"
  repository        = "https://grafana.github.io/loki/charts"
  chart             = "loki-stack"
  namespace         = "loki-stack"
  create_namespace  = true
  values      = [yamlencode(local.values)]
  depends_on = [var.dep]
  set {
    name  = "loki.nodeSelector.group_name"
    value = "service"
  }
  set {
    name  = "promtail.tolerations[0].operator"
    value = "Exists"
  }
  set {
    name  = "promtail.tolerations[0].effect"
    value = "NoSchedule"
  }
}
