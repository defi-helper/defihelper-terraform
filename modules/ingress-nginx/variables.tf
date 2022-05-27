variable "node_selector" {
  type = map(string)
}

variable "load_balancer_ip" {
  type = string
}

variable "nginx_ingress_replicacount" {
  type = number
}

variable "nginx_ingress_backend_replicacount" {
  type = number
}

variable "nginx_ingress_replicacount_max" {
  type = number
}

variable "nginx_ingress_cpu_request" {
  type = string
}

variable "nginx_ingress_memory_request" {
  type = string
}

