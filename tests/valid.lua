ecs.system("Movement", { "Position", "Velocity" }, 
function(entity, components, dt)
    print("Starting system")
    local position = components["Position"]
    local velocity = components["Velocity"]
    print("Declared components")

    local x = position:readFloat("x")
    local y = position:readFloat("y")
    print(x)
    print(y)

    local vx = velocity:readFloat("x")
    local vy = velocity:readFloat("y")
    print(vx)
    print(vy)

    position:writeFloat("x", x + vx * dt)
    position:writeFloat("y", y + vy * dt)
end
)
