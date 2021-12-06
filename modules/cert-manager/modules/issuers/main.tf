locals {
  issuers = {
    staging = {
      apiVersion = "cert-manager.io/v1"
      kind = "ClusterIssuer"
      metadata = {
        name = "letsencrypt-staging"
      }
      spec = {
        acme = {
          email = var.staging_email
          server = "https://acme-staging-v02.api.letsencrypt.org/directory"
          privateKeySecretRef = {
            name = "letsencrypt-staging"
          }
          solvers = [
            {
              http01 = {
                ingress = {
                  class = "nginx"
                }
              }
            }
          ]
        }
      }
    }
    production = {
      apiVersion = "cert-manager.io/v1"
      kind = "ClusterIssuer"
      metadata = {
        name = "letsencrypt-production"
      }
      spec = {
        acme = {
          email = var.production_email
          server = "https://acme-v02.api.letsencrypt.org/directory"
          privateKeySecretRef = {
            name = "letsencrypt-production"
          }
          solvers = [
            {
              http01 = {
                ingress = {
                  class = "nginx"
                }
              }
            }
          ]
        }
      }
    }
  }
}

resource "kubectl_manifest" "issuers" {
  for_each = local.issuers
  yaml_body = yamlencode(each.value)
}
