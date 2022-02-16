resource "helm_release" "gitlab-runner" {
  count      = var.enable_gitlab_runner ? 1 : 0
  name       = "gitlab-runner"
  namespace  = "gitlab"
  repository = "https://charts.gitlab.io/"
  chart      = "gitlab-runner"
  timeout    = 1800
  atomic     = true
  values     = [file("modules/gitlab-runner/values.yaml")]

  set {
    name  = "runnerRegistrationToken"
    value = var.gitlabRunnerRegistrationToken
  }

  set {
    name  = "gitlabUrl"
    value = "https://adcorn-prod.gitlab.yandexcloud.net"
  }
  set {
    name  = "runners.env.DOCKER_AUTH_CONFIG"
    value = var.gitlab_runner_docker_io_auth
  }
  set {
    name  = "runners.tags"
    value = var.gitlab_runner_tags
  }
}
