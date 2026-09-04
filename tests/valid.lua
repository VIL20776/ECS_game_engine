ecs.system {
    name = "Movement",
    query = { "Position", "Velocity" },
    update = function(entity, components, dt)
        local position = components["Position"]
        local velocity = components["Velocity"]
        
        position.x = position.x + velocity.x * dt
        position.y = position.y + velocity.y * dt
    end
}
