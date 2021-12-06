locals {
  values = {
    persistence = {
      enabled      = true
      storageClass = var.storage_class
      size         = var.storage_size
    }
    storageClass = {
      name          = "nfs-client"
      reclaimPolicy = "Delete"
    }
    nodeSelector = var.node_selector
    "tolerations" =[{
      "effect" = "NoSchedule"
      "key" = "node-role.kubernetes.io/nfs"
      "operator" = "Equal"
      "value" = "true"
    }
    ]
  }
}

resource "kubernetes_namespace" "nfs-server-provisioner" {
  metadata {
    name = "nfs-server-provisioner"
  }
}

resource "helm_release" "nfs-server-provisioner" {
  name       = "nfs-server-provisioner"
  repository = "https://charts.helm.sh/stable/"
  chart      = "nfs-server-provisioner"
  namespace  = kubernetes_namespace.nfs-server-provisioner.metadata[0].name

  values = [yamlencode(local.values)]
}
