resource "kubernetes_namespace" "gitlab" {
  metadata {
    name = "gitlab"
  }
}


resource "kubernetes_secret" "git-https-certificate" {
  metadata {
    name      = "git-https-certificate"
    namespace = "gitlab"
  }

  data = {
    "tls.crt" = var.ssl_1iu_ru_crt
    "tls.key" = var.ssl_1iu_ru_key
  }
}

resource "kubernetes_secret" "git-smtp-password" {
  metadata {
    name      = "git-smtp-password"
    namespace = "gitlab"
  }

  data = {
    "password" = var.gitlab_smtp_password
  }
}

resource "kubernetes_secret" "git-s3cfg" {
  metadata {
    name      = "git-s3cfg"
    namespace = "gitlab"
  }
  data = {
    "config" = var.git-s3cfg
  }
  type = "Opaque"
}

# resource "kubernetes_secret" "gitlabstorage-static-key" {
#   metadata {
#     name      = "gitlabstorage-static-key"
#     namespace = "gitlab"
#   }

#   data = {
#     "connection" = var.gitlabstorage-static-key
#   }
# }

# resource "kubernetes_secret" "gitlab-redis-password" {
#   metadata {
#     name      = "gitlab-redis-password"
#     namespace = "gitlab"
#   }
#   data = {
#     redis-password = var.redis_password
#   }
#   type = "Opaque"
# }

resource "kubernetes_secret" "gitlab-postgresql-password" {
  metadata {
    name      = "gitlab-postgresql-password"
    namespace = "gitlab"
  }
  data = {
    postgresql-password = var.pg_git_password
  }
  type = "Opaque"
}

locals {
  gitlab = {
    "gitlab" = {
#      "webservice" = {
#        "resources" = {
#          "limits" = {
#            "cpu"    = 2
#            "memory" = "2G"
#          }
#          "requests" = {
#            "cpu"    = "900m"
#            "memory" = "1.5G"
#          }
#        }
#      }
#      "sidekiq" = {
#        "resources" = {
#          "limits" = {
#            "cpu"    = 2
#            "memory" = "2G"
#          }
#          "requests" = {
#            "cpu"    = "900m"
#            "memory" = "1.5G"
#          }
#        }
#      }
    }
  }

  le_annotations = [
    {
      name  = "ingress.annotations.cert-manager\\.io/cluster-issuer"
      value = "letsencrypt-production"
    }
  ]
}

resource "helm_release" "gitlab" {
  count      = var.enable_gitlab ? 1 : 0
  name       = "gitlab"
  namespace  = "gitlab"
  repository = "https://charts.gitlab.io/"
  version    = var.version_gitlab
  chart      = "gitlab"
  #  create_namespace = true
  #  values  = [yamlencode(local.values)]
  timeout = 1800
  #values  = [file("modules/gitlab/values.yaml")]
  values = [yamlencode(local.gitlab)]

  dynamic "set" {
    for_each = [for a in local.le_annotations : {
      name  = a.name
      value = a.value
    }]
    content {
      name  = set.value.name
      value = set.value.value
    }
  }
  set {
    name  = "global.edition"
    value = "ee"
  }

  set {
    name  = "global.nodeSelector.group_name"
    value = "service"
  }

  set {
    name  = "global.hosts.domain"
    value = var.cluster_domain
  }

  set {
    name  = "global.ingress.class"
    value = ""
  }
  set {
    name  = "global.ingress.enabled"
    value = true
  }

  set {
    name  = "nginx-ingress.enabled"
    value = false
  }
  set {
    name  = "global.ingress.configureCertmanager"
    value = false
  }

  set {
#    name  = "certmanager.createCustomResource"
    name  = "certmanager.installCRDs"
    value = false
  }

  set {
    name  = "global.ingress.tls.secretName"
    value = var.gitlab_certificate_secret_name
  }
  set {
    name  = "certmanager.install"
    value = false
  }

  ####minio
  set {
    name  = "global.minio.enabled"
    value = true
  }

  set {
    name  = "minio.persistence.size"
    value = "50Gi"
  }
  # set {
  #   name  = "global.appConfig.object_store.enabled"
  #   value = true
  # }
  # set {
  #   name  = "global.appConfig.object_store.connection.secret"
  #   value = "gitlabstorage-static-key"
  # }

  # set {
  #   name  = "global.appConfig.lfs.bucket"
  #   value = "gitlab-lfs-storage"
  # }

  # set {
  #   name  = "global.appConfig.lfs.connection.secret"
  #   value = "gitlabstorage-static-key"
  # }
  # set {
  #   name  = "global.appConfig.lfs.connection.key"
  #   value = "connection"
  # }
  ####registry
  # set {
  #   name  = "gitlab.unicorn.registry.enabled"
  #   value = false
  # }
  ####redis
  set {
    name  = "redis.install"
    value = true
  }
  # set {
  #   name  = "global.redis.host"
  #   value = var.gitlab_redis_host
  # }

  # set {
  #   name  = "global.redis.port"
  #   value = var.gitlab_redis_port
  # }

  # set {
  #   name  = "global.redis.password.secret"
  #   value = "gitlab-redis-password"
  # }

  # set {
  #   name  = "global.redis.password.key"
  #   value = "redis-password"
  # }

  ####prometheus
  set {
    name  = "prometheus.install"
    value = false
  }
  ####psql

  set {
    name  = "postgresql.install"
    value = false
  }
  set {
    name  = "global.psql.host"
    value = var.gitlab_psql_host
  }

  set {
    name  = "global.psql.port"
    value = var.gitlab_psql_port
  }
  set {
    name  = "global.psql.password.secret"
    value = "gitlab-postgresql-password"
  }
  set {
    name  = "global.psql.password.key"
    value = "postgresql-password"
  }

  set {
    name  = "global.psql.database"
    value = var.pg_git_name
  }

  set {
    name  = "global.psql.username"
    value = var.pg_git_name
  }
  ####smtp
  set {
    name  = "global.smtp.enabled"
    value = "true"
  }

  set {
    name  = "global.smtp.authentication"
    value = "true"
  }

  set {
    name  = "global.email.display_name"
    value = "1IU GitLab"
  }

  set {
    name  = "global.email.from"
    value = var.gitlab_email_from
  }
  set {
    name  = "global.smtp.address"
    value = var.gitlab_smtp_address
  }
  set {
    name  = "global.smtp.port"
    value = var.gitlab_smtp_port
  }

  set {
    name  = "global.smtp.user_name"
    value = var.gitlab_email_from
  }

  set {
    name  = "global.smtp.password.secret"
    value = "git-smtp-password"
  }

  set {
    name  = "global.smtp.password.key"
    value = "password"
  }

  set {
    name  = "global.smtp.authentication"
    value = "login"
  }

  set {
    name  = "global.smtp.domain"
    value = "yandex.ru"
  }

  set {
    name  = "global.email.from"
    value = var.gitlab_email_from
  }

  set {
    name  = "global.smtp.tls"
    value = "true"
  }

  set {
    name  = "global.smtp.starttls_auto"
    value = "true"
  }

  # set {
  #   name  = "global.smtp.openssl_verify_mode"
  #   value = "none"
  # }


  ####runner

  set {
    name  = "gitlab-runner.install"
    value = false
  }


  ####backup
  set {
    name  = "gitlab.task-runner.backups.cron.enabled"
    value = var.backups_cron_git
  }

  set {
    name  = "gitlab.task-runner.backups.cron.schedule"
    value = "0 0 * * *"
  }

  set {
    name  = "gitlab.task-runner.persistence.enabled"
    value = "true"
  }

  # set {
  #   name  = "gitlab.task-runner.backups.cron.persistence.size"
  #   value = "15Gi"
  # }

  set {
    name  = "gitlab.task-runner.backups.objectStorage.config.secret"
    value = "git-s3cfg"
  }

  set {
    name  = "global.appConfig.backups.bucket"
    value = var.gitlab-backup-storage
  }
  set {
    name  = "global.appConfig.backups.tmpBucket"
    value = var.gitlab-tmp-storage
  }
}
