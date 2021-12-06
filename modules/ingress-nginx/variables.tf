variable "node_selector" {
  type = map(string)
}

variable "load_balancer_ip" {
  type = string
}

variable "nginx_ingress_replicacount" {
  type = number
}
