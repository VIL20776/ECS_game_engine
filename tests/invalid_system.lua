ecs.system("broken", { "MissingComponent" }, function(entity, components, dt)
    return false
end)
