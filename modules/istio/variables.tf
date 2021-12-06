variable "configs" {}

variable "node_selector" {
  type = map(string)
}

variable "cluster_domain" {
  type = string
}

variable "istio_auth" {
    type = string
}
